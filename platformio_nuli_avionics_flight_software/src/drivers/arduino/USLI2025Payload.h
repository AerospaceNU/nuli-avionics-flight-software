#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H

#include "AprsModulation.h"
#include "Avionics.h"
#include "../../core/HardwareAbstraction.h"

#define BUFF_SIZE (20*8)

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

    explicit USLI2025Payload() = default;

    void setup(HardwareAbstraction* hardware);

    void loopOnce(uint32_t runtime, uint32_t dt, double altitudeM, double velocityMS, double netAccelMSS, double orientationDeg, double temp, double batteryVoltage);

    void deployLegs() const;

    void sendTransmission(uint32_t runtime);

    const char* getTransmitStr();

private:
    void updateGroundData(double temp, double batteryVoltage, double orientationRad);

    void begin(const char* callsign);

    void addStr(const char* str);

    void addInt(int num);

    void calculateSurvivability(uint32_t runTimeMs, double acceleration);

    void updateLandingBuffers(double altitude, double velocity, double acceleration, uint32_t timeMs);

    bool checkLanded();

    uint16_t getLandingVelocity();

    uint16_t getLandingAccelG();

    HardwareAbstraction* m_hardware = nullptr;

    uint32_t m_liftoffTime = 0;

    char m_transmitBuffer[300]{};
    char* m_transmitStringLocation = m_transmitBuffer;

    uint32_t m_nextTransmitTime = 0;
    uint32_t m_nextDeployTime = 0;
    PayloadData m_payloadData;

    FlightState_e m_flightState = PRE_FLIGHT;
    double m_takeoffThresholdM = 230.0;
    uint32_t m_stateTimer = 0;

    // Survivability vars]
    // tracks 10 seconds worth
    int m_redLine[10] = {300, 260, 230, 210, 200, 195, 190, 185, 180, 170};
    int m_accelThreshold[10] = {90, 85, 80, 75, 70, 65, 60, 55, 50, 45};
    uint32_t m_survivabilityTotalTime = 0;
    int m_survivabilityStartTime = -1;

    int16_t altitudeBuff[BUFF_SIZE];
    int32_t timeMsBuff[BUFF_SIZE];
    int16_t accelerationBuff[BUFF_SIZE];
    int16_t bufferIndex = BUFF_SIZE;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_USLI2025PAYLOAD_H
