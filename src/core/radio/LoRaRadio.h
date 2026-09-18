#pragma once

#include <Arduino.h>

class LoRaRadio {
public:
    using YieldCallback = void(*)();

    virtual ~LoRaRadio() = default;

    virtual bool begin(uint32_t frequency) = 0;
    virtual void end() = 0;

    virtual int beginPacket(int implicitHeader = 0) = 0;
    virtual int endPacket(bool async = false) = 0;
    virtual bool isTxBusy() = 0;
    virtual size_t write(uint8_t byte) = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;

    virtual void receive(int size = 0) = 0;
    virtual int parsePacket(int size = 0) = 0;
    virtual int currentRssi() = 0;
    virtual int packetRssi() = 0;
    virtual float packetSnr() = 0;
    virtual const uint8_t* packetBuffer() const = 0;

    virtual void setFrequency(uint32_t frequency) = 0;
    virtual uint32_t getFrequency() = 0;
    virtual void setTxPower(int level) = 0;
    virtual int8_t getTxPower() = 0;
    virtual void setSpreadingFactor(int sf) = 0;
    virtual uint8_t getSpreadingFactor() = 0;
    virtual void setSignalBandwidth(uint32_t bandwidth) = 0;
    virtual uint32_t getSignalBandwidth() = 0;
    virtual void setCodingRate4(int denominator) = 0;
    virtual uint8_t getCodingRate4() = 0;
    virtual void setPreambleLength(long length) = 0;
    virtual long getPreambleLength() const = 0;
    virtual void setInvertIQ(bool invert) = 0;
    virtual bool getInvertIQ() const = 0;

    virtual bool isRadioOnline() = 0;
    virtual float getAirtime(uint16_t written) = 0;
    virtual uint32_t getBitrate() = 0;
    virtual bool lowDataRateEnabled() const = 0;
    virtual uint8_t readRegister(uint16_t address) = 0;
    virtual uint16_t getDeviceErrors() = 0;
    virtual uint8_t getStatus() = 0;
    virtual uint8_t getPacketType() = 0;
    virtual uint16_t getIrqFlags() = 0;

    virtual void setYieldCallback(YieldCallback callback) = 0;
    virtual void sleep() = 0;

    volatile bool packetAvailable = false;
};