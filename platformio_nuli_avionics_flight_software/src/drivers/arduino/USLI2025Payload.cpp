#include "USLI2025Payload.h"

USLI2025Payload::USLI2025Payload(const char* callsign) : m_aprsModulation(m_transmitPin, callsign) {

}

//#include "iostream"

void USLI2025Payload::loopOnce(uint32_t runtime, uint32_t dt, double altitudeM, double velocityMS, double netAccelMSS, double orientationDeg, double temp, double batteryVoltage) {

    updateGroundData(temp, batteryVoltage, orientationDeg);

    /**
     * Pre-flight state
     * Detect launch
     */
    if (m_flightState == PRE_FLIGHT) {
        if (altitudeM > takeoffThresholdM) {
            if (takeoffTimer == 0) {
                takeoffTimer = runtime + 1000;
            }
        } else {
            takeoffTimer = 0;
        }

        if (takeoffTimer != 0 && runtime > takeoffTimer) {
            m_flightState = FLIGHT;
            takeoffTimer = runtime + (1000 * 60 * 3);
        }
    }

        /**
         * Flight state
         * Record data
         */
    else if (m_flightState == FLIGHT) {
        if (altitudeM > m_payloadData.alt) {
            m_payloadData.alt = (int32_t) altitudeM;
        }

        if (velocityMS > m_payloadData.maxVel) {
            m_payloadData.maxVel = (int32_t) velocityMS;
        }

        if (runtime > takeoffTimer) {
            m_payloadData.time = (int32_t) runtime;
            m_flightState = LANDED;
        }
    }

    /**
     * Landed state
     * Transmit data
     */
    else if (m_flightState == LANDED) {
        if (runtime > m_nextDeployTime) {
            m_nextDeployTime = runtime + 15000;
            deployLegs();
            delay(2000);
        }

        if (runtime > m_nextTransmitTime) {
            m_nextTransmitTime = runtime + 5000;
            sendTransmission(runtime);
        }
    }
        // Should never happen
    else {
        m_flightState = PRE_FLIGHT;
    }
}

void USLI2025Payload::updateGroundData(double temp, double batteryVoltage, double orientationRad) {
    m_payloadData.ort = (int32_t) orientationRad;
    m_payloadData.battery = (int32_t) batteryVoltage * 10;
    m_payloadData.temp = (int32_t) temp;
}

void USLI2025Payload::setup() {
    pinMode(m_deployPin, OUTPUT);
    digitalWrite(m_deployPin, LOW);
    m_aprsModulation.setup();
}


void USLI2025Payload::deployLegs() const {
    digitalWrite(m_deployPin, HIGH);
    delay(350);
    digitalWrite(m_deployPin, LOW);
}

void USLI2025Payload::begin(const char* callsign) {
    m_transmitStringLocation = m_transmitBuffer;
    addStr(callsign);
}

void USLI2025Payload::addStr(const char* str) {
    m_transmitStringLocation += sprintf(m_transmitStringLocation, "%s", str);
}

void USLI2025Payload::addInt(int num) {
    m_transmitStringLocation += sprintf(m_transmitStringLocation, ";%d", num);
}

void USLI2025Payload::sendTransmission(uint32_t runtime) {
    begin(m_aprsModulation.getCallsign());
    addInt((int) double(double(runtime - m_payloadData.time) / 1000.0));
    addInt(m_payloadData.temp);
    addInt(m_payloadData.battery);
    addInt(m_payloadData.alt);
    addInt(m_payloadData.ort);
    addInt(m_payloadData.maxVel);
    addInt(m_payloadData.landVel);
    addInt(m_payloadData.accel);
    addInt(m_payloadData.suviv);
    addStr(m_aprsModulation.getCallsign());
    m_aprsModulation.transmit(m_transmitBuffer);
}

const char* USLI2025Payload::getTransmitStr() {
    return m_transmitBuffer;
}








