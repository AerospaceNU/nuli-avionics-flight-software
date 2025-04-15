#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H

#include "AprsModulation.h"
#include "Avionics.h"
#include "HardwareAbstraction.h"
#include "Arduino.h"

enum FlightState_e {
    PRE_FLIGHT,
    FLIGHT,
    LANDED,
};

struct PayloadData {
    uint32_t time = 1;
    int32_t temp = 273;
    int32_t battery = 12;
    int32_t alt = 0;
    int32_t ort = 0;
    int32_t maxVel = 0;
    int32_t landVel = 5;
    int32_t accel = 8;
    int32_t suviv = 99;
};

class USLI2025Payload {
public:
    bool m_transmitAllowed = true;

    explicit USLI2025Payload(const char* callsign);

    void setup(HardwareAbstraction *hardware);

    void loopOnce(uint32_t runtime, uint32_t dt, double altitudeM, double velocityMS, double netAccelMSS, double orientationDeg, double temp, double batteryVoltage);

    void deployLegs() const;

    void sendTransmission(uint32_t runtime);

    const char* getTransmitStr();

private:
    void updateGroundData(double temp, double batteryVoltage, double orientationRad);

    void begin(const char* callsign);

    void addStr(const char* str);

    void addInt(int num);

    HardwareAbstraction *m_hardware = nullptr;

    char m_transmitBuffer[300];
    char* m_transmitStringLocation = m_transmitBuffer;
    const uint8_t m_transmitPin = A0;

    AprsModulation m_aprsModulation;
    uint32_t m_nextTransmitTime = 0;
    uint32_t m_nextDeployTime = 0;
    PayloadData m_payloadData;

    FlightState_e m_flightState = PRE_FLIGHT;
    double takeoffThresholdM = 230.0;
    uint32_t takeoffTimer = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
