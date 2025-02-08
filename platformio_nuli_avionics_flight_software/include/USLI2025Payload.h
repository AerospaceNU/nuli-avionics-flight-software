#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H

#include "AprsModulation.h"

#ifdef PLATFORMIO
#include "Arduino.h"
#else

#include <thread>
#include <chrono>

#define delay(time) std::this_thread::sleep_for(std::chrono::seconds(time));

inline void pinMode(int m_deployPin, int OUTPUT) {}

inline void digitalWrite(int m_deployPin, int LOW) {}

inline int32_t A0 = 0;
inline int32_t OUTPUT = 0;
inline int32_t LOW = 0;
inline int32_t HIGH = 0;


#endif

#include "Avionics.h"


enum FlightState_e {
    PRE_FLIGHT,
    FLIGHT,
    LANDED,
};

struct PayloadData {
    uint32_t time = 0;
    int32_t temp = 0;
    int32_t battery = 0;
    int32_t alt = 0;
    int32_t ort = 0;
    int32_t maxVel = 0;
    int32_t landVel = 0;
    int32_t accel = 0;
    int32_t suviv = 0;
};

class USLI2025Payload {
public:
    explicit USLI2025Payload(const char* callsign);

    void setup();

    void loopOnce(uint32_t runtime, uint32_t dt, double altitudeM, double velocityMS, double netAccelMSS, double orientationRad, double temp, double batteryVoltage);

    void deployLegs() const;

    void sendTransmission(uint32_t runtime);

private:
    void updateGroundData(double temp, double batteryVoltage, double orientationRad);

    void begin(const char* callsign);

    void addStr(const char* str);

    void addInt(int num);

    char m_transmitBuffer[300];
    char* m_transmitStringLocation = m_transmitBuffer;
    const uint8_t m_transmitPin = A0;

    AprsModulation m_aprsModulation;
    const uint8_t m_deployPin = 11;
    uint32_t m_nextTransmitTime = 0;
    uint32_t m_nextDeployTime = 0;
    PayloadData m_payloadData;

    FlightState_e m_flightState = PRE_FLIGHT;
    bool takenOff = false;
    double takeoffThresholdM = 500;
    uint32_t takeoffTimer = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
