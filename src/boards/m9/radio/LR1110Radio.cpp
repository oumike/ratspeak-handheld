#include "LR1110Radio.h"
#include "hal/SharedSPIBus.h"
#include <algorithm>
#include <cmath>

LR1110Radio* LR1110Radio::_instance = nullptr;
portMUX_TYPE LR1110Radio::_stateMux = portMUX_INITIALIZER_UNLOCKED;

LR1110Radio::LR1110Radio(SPIClass* spi, int chipSelect, int irq, int reset, int busy)
    : _spi(spi),
      _module(chipSelect, irq, reset, busy, *spi, SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0)),
      _radio(&_module),
      _irq(irq) {}

bool LR1110Radio::apply(int16_t result, const char* operation) {
    if (result == RADIOLIB_ERR_NONE) return true;
    Serial.printf("[LR1110] %s failed: %d\n", operation, result);
    return false;
}

void LR1110Radio::configureRfSwitch() {
    static const uint32_t pins[Module::RFSWITCH_MAX_PINS] = {
        RADIOLIB_LR11X0_DIO5,
        RADIOLIB_LR11X0_DIO6,
        RADIOLIB_NC,
        RADIOLIB_NC,
        RADIOLIB_NC,
    };
    static const Module::RfSwitchMode_t table[] = {
        {LR11x0::MODE_STBY, {LOW, LOW}},
        {LR11x0::MODE_RX, {HIGH, LOW}},
        {LR11x0::MODE_TX, {HIGH, HIGH}},
        {LR11x0::MODE_TX_HP, {LOW, HIGH}},
        {LR11x0::MODE_TX_HF, {LOW, LOW}},
        {LR11x0::MODE_GNSS, {LOW, LOW}},
        {LR11x0::MODE_WIFI, {LOW, LOW}},
        END_OF_MODE_TABLE,
    };
    _radio.setRfSwitchTable(pins, table);
}

bool LR1110Radio::begin(uint32_t frequency) {
    _instance = this;
    _frequency = frequency;

    SharedSPILock bus;
    if (!bus.locked()) return false;
    int16_t result = _radio.begin(
        _frequency / 1000000.0f,
        _bandwidth / 1000.0f,
        _spreadingFactor,
        _codingRate,
        RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE,
        _txPower,
        (uint16_t)_preambleLength,
        LORA_TCXO_VOLTAGE);
    if (result != RADIOLIB_ERR_NONE) {
        delay(150);
        result = _radio.begin(
            _frequency / 1000000.0f,
            _bandwidth / 1000.0f,
            _spreadingFactor,
            _codingRate,
            RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE,
            _txPower,
            (uint16_t)_preambleLength,
            LORA_TCXO_VOLTAGE);
    }
    if (!apply(result, "begin")) return false;

    configureRfSwitch();
    if (!apply(_radio.setCRC(2), "set CRC")) return false;
    if (!apply(_radio.explicitHeader(), "set explicit header")) return false;
    if (!apply(_radio.setRxBoostedGainMode(true), "enable boosted RX")) return false;

    LR11x0VersionInfo_t version = {};
    if (_radio.getVersionInfo(&version) == RADIOLIB_ERR_NONE) {
        Serial.printf("[LR1110] hw=0x%02X device=0x%02X fw=%u.%u\n",
                      version.hardware, version.device, version.fwMajor, version.fwMinor);
    }
    _online = true;
    pinMode(_irq, INPUT);
    attachInterrupt(digitalPinToInterrupt(_irq), onDio1Rise, RISING);
    receive();
    return true;
}

void LR1110Radio::end() {
    detachInterrupt(digitalPinToInterrupt(_irq));
    portENTER_CRITICAL(&_stateMux);
    _operation = Operation::Idle;
    _txDone = false;
    packetAvailable = false;
    portEXIT_CRITICAL(&_stateMux);
    sleep();
    _online = false;
    _instance = nullptr;
}

int LR1110Radio::beginPacket(int implicitHeader) {
    _txLength = 0;
    if (implicitHeader) {
        Serial.println("[LR1110] Implicit header TX is not supported by this adapter");
        return 0;
    }
    return _online ? 1 : 0;
}

size_t LR1110Radio::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t LR1110Radio::write(const uint8_t* buffer, size_t size) {
    if (!buffer || _txLength >= MAX_PACKET_SIZE) return 0;
    const size_t writable = std::min(size, (size_t)MAX_PACKET_SIZE - _txLength);
    memcpy(_txBuffer + _txLength, buffer, writable);
    _txLength += writable;
    return writable;
}

int LR1110Radio::endPacket(bool async) {
    if (!_online || _txLength == 0) return 0;
    SharedSPILock bus;
    if (!bus.locked()) return 0;

    if (!async) {
        portENTER_CRITICAL(&_stateMux);
        _operation = Operation::Transmitting;
        _txDone = false;
        packetAvailable = false;
        portEXIT_CRITICAL(&_stateMux);
        const int16_t result = _radio.transmit(_txBuffer, _txLength);
        portENTER_CRITICAL(&_stateMux);
        _operation = Operation::Idle;
        portEXIT_CRITICAL(&_stateMux);
        return apply(result, "transmit") ? 1 : 0;
    }

    portENTER_CRITICAL(&_stateMux);
    _operation = Operation::Transmitting;
    _txDone = false;
    packetAvailable = false;
    portEXIT_CRITICAL(&_stateMux);
    const int16_t result = _radio.startTransmit(_txBuffer, _txLength);
    if (result != RADIOLIB_ERR_NONE) {
        portENTER_CRITICAL(&_stateMux);
        _operation = Operation::Idle;
        portEXIT_CRITICAL(&_stateMux);
        apply(result, "start transmit");
        return 0;
    }
    _txStartedMs = millis();
    _txBudgetMs = (uint32_t)ceilf(getAirtime(_txLength)) + 1000;
    return 1;
}

bool LR1110Radio::isTxBusy() {
    portENTER_CRITICAL(&_stateMux);
    const bool transmitting = _operation == Operation::Transmitting;
    const bool txDone = _txDone;
    portEXIT_CRITICAL(&_stateMux);
    if (!transmitting) return false;
    if (!txDone && millis() - _txStartedMs <= _txBudgetMs) {
        if (_yieldCallback) _yieldCallback();
        return true;
    }

    portENTER_CRITICAL(&_stateMux);
    _operation = Operation::Idle;
    _txDone = false;
    portEXIT_CRITICAL(&_stateMux);
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.finishTransmit(), "finish transmit");
    return false;
}

void LR1110Radio::receive(int) {
    if (!_online) return;
    portENTER_CRITICAL(&_stateMux);
    const bool transmitting = _operation == Operation::Transmitting;
    if (!transmitting) {
        _operation = Operation::Receiving;
        _txDone = false;
        packetAvailable = false;
    }
    portEXIT_CRITICAL(&_stateMux);
    if (transmitting) return;
    SharedSPILock bus;
    if (!bus.locked()) return;
    if (!apply(_radio.startReceive(), "start receive")) {
        portENTER_CRITICAL(&_stateMux);
        _operation = Operation::Idle;
        portEXIT_CRITICAL(&_stateMux);
    }
}

int LR1110Radio::parsePacket(int) {
    if (!_online) return 0;
    portENTER_CRITICAL(&_stateMux);
    _operation = Operation::Idle;
    packetAvailable = false;
    portEXIT_CRITICAL(&_stateMux);
    SharedSPILock bus;
    if (!bus.locked()) return 0;

    const size_t packetLength = _radio.getPacketLength();
    if (packetLength == 0 || packetLength > MAX_PACKET_SIZE) {
        _radio.finishReceive();
        return 0;
    }
    const int16_t result = _radio.readData(_rxBuffer, packetLength);
    _lastPacketRssi = (int)lroundf(_radio.getRSSI());
    _lastPacketSnr = _radio.getSNR();
    if (result != RADIOLIB_ERR_NONE) {
        apply(result, "read packet");
        return 0;
    }
    return (int)packetLength;
}

int LR1110Radio::currentRssi() {
    portENTER_CRITICAL(&_stateMux);
    const bool transmitting = _operation == Operation::Transmitting;
    portEXIT_CRITICAL(&_stateMux);
    if (!_online || transmitting) return _lastPacketRssi;
    SharedSPILock bus(pdMS_TO_TICKS(20));
    if (!bus.locked()) return _lastPacketRssi;
    return (int)lroundf(_radio.getRSSI(false, true));
}

int LR1110Radio::packetRssi() { return _lastPacketRssi; }
float LR1110Radio::packetSnr() { return _lastPacketSnr; }

void LR1110Radio::setFrequency(uint32_t frequency) {
    _frequency = frequency;
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setFrequency(frequency / 1000000.0f), "set frequency");
}

void LR1110Radio::setTxPower(int level) {
    _txPower = constrain(level, -9, 22);
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setOutputPower(_txPower), "set TX power");
}

void LR1110Radio::setSpreadingFactor(int sf) {
    _spreadingFactor = constrain(sf, 5, 12);
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setSpreadingFactor(_spreadingFactor), "set spreading factor");
}

void LR1110Radio::setSignalBandwidth(uint32_t bandwidth) {
    _bandwidth = bandwidth;
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setBandwidth(bandwidth / 1000.0f), "set bandwidth");
}

void LR1110Radio::setCodingRate4(int denominator) {
    _codingRate = constrain(denominator, 5, 8);
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setCodingRate(_codingRate), "set coding rate");
}

void LR1110Radio::setPreambleLength(long length) {
    _preambleLength = std::max(1L, length);
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.setPreambleLength((size_t)_preambleLength), "set preamble");
}

void LR1110Radio::setInvertIQ(bool invert) {
    _invertIq = invert;
    if (!_online) return;
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.invertIQ(invert), "set IQ inversion");
}

float LR1110Radio::getAirtime(uint16_t written) {
    if (!_online) return 0;
    return _radio.getTimeOnAir(written) / 1000.0f;
}

uint32_t LR1110Radio::getBitrate() {
    if (_spreadingFactor == 0 || _bandwidth == 0 || _codingRate < 5) return 0;
    const float bitrate = _spreadingFactor * (4.0f / _codingRate) * _bandwidth /
        (float)(1UL << _spreadingFactor);
    return std::max(1UL, (unsigned long)bitrate);
}

bool LR1110Radio::lowDataRateEnabled() const {
    return (1000.0f * (1UL << _spreadingFactor) / _bandwidth) > 16.0f;
}

uint8_t LR1110Radio::getStatus() {
    portENTER_CRITICAL(&_stateMux);
    const bool transmitting = _operation == Operation::Transmitting;
    portEXIT_CRITICAL(&_stateMux);
    return transmitting ? 0x60 : 0x50;
}

uint16_t LR1110Radio::getIrqFlags() {
    if (!_online) return 0;
    SharedSPILock bus(pdMS_TO_TICKS(20));
    return bus.locked() ? (uint16_t)_radio.getIrqFlags() : 0;
}

void LR1110Radio::sleep() {
    if (!_online) return;
    portENTER_CRITICAL(&_stateMux);
    _operation = Operation::Idle;
    _txDone = false;
    packetAvailable = false;
    portEXIT_CRITICAL(&_stateMux);
    SharedSPILock bus;
    if (bus.locked()) apply(_radio.sleep(), "sleep");
}

void IRAM_ATTR LR1110Radio::onDio1Rise() {
    if (!_instance) return;
    portENTER_CRITICAL_ISR(&_stateMux);
    if (_instance->_operation == Operation::Transmitting) {
        _instance->_txDone = true;
    } else if (_instance->_operation == Operation::Receiving) {
        _instance->packetAvailable = true;
    }
    portEXIT_CRITICAL_ISR(&_stateMux);
}
