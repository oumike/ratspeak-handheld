#pragma once

#include <RadioLib.h>
#include <SPI.h>
#include "radio/LoRaRadio.h"
#include "config/BoardConfig.h"

class LR1110Radio : public LoRaRadio {
public:
    LR1110Radio(SPIClass* spi, int chipSelect, int irq, int reset, int busy);

    bool begin(uint32_t frequency) override;
    void end() override;

    int beginPacket(int implicitHeader = 0) override;
    int endPacket(bool async = false) override;
    bool isTxBusy() override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t* buffer, size_t size) override;

    void receive(int size = 0) override;
    int parsePacket(int size = 0) override;
    int currentRssi() override;
    int packetRssi() override;
    float packetSnr() override;
    const uint8_t* packetBuffer() const override { return _rxBuffer; }

    void setFrequency(uint32_t frequency) override;
    uint32_t getFrequency() override { return _frequency; }
    void setTxPower(int level) override;
    int8_t getTxPower() override { return _txPower; }
    void setSpreadingFactor(int sf) override;
    uint8_t getSpreadingFactor() override { return _spreadingFactor; }
    void setSignalBandwidth(uint32_t bandwidth) override;
    uint32_t getSignalBandwidth() override { return _bandwidth; }
    void setCodingRate4(int denominator) override;
    uint8_t getCodingRate4() override { return _codingRate; }
    void setPreambleLength(long length) override;
    long getPreambleLength() const override { return _preambleLength; }
    void setInvertIQ(bool invert) override;
    bool getInvertIQ() const override { return _invertIq; }

    bool isRadioOnline() override { return _online; }
    float getAirtime(uint16_t written) override;
    uint32_t getBitrate() override;
    bool lowDataRateEnabled() const override;
    uint8_t readRegister(uint16_t) override { return 0; }
    uint16_t getDeviceErrors() override { return 0; }
    uint8_t getStatus() override;
    uint8_t getPacketType() override { return 0x01; }
    uint16_t getIrqFlags() override;

    void setYieldCallback(YieldCallback callback) override { _yieldCallback = callback; }
    void sleep() override;

private:
    enum class Operation : uint8_t { Idle, Receiving, Transmitting };

    static void IRAM_ATTR onDio1Rise();
    bool apply(int16_t result, const char* operation);
    void configureRfSwitch();

    SPIClass* _spi;
    Module _module;
    LR1110 _radio;
    int _irq;
    bool _online = false;
    volatile Operation _operation = Operation::Idle;
    volatile bool _txDone = false;
    uint32_t _txStartedMs = 0;
    uint32_t _txBudgetMs = 0;
    uint8_t _txBuffer[MAX_PACKET_SIZE] = {};
    size_t _txLength = 0;
    uint8_t _rxBuffer[MAX_PACKET_SIZE] = {};
    int _lastPacketRssi = 0;
    float _lastPacketSnr = 0;
    uint32_t _frequency = LORA_DEFAULT_FREQ;
    uint32_t _bandwidth = LORA_DEFAULT_BW;
    uint8_t _spreadingFactor = LORA_DEFAULT_SF;
    uint8_t _codingRate = LORA_DEFAULT_CR;
    int8_t _txPower = LORA_DEFAULT_TX_POWER;
    long _preambleLength = LORA_DEFAULT_PREAMBLE;
    bool _invertIq = false;
    YieldCallback _yieldCallback = nullptr;

    static LR1110Radio* _instance;
    static portMUX_TYPE _stateMux;
};
