#include "USLI2025Payload.h"
#include <cstdio>
#include "../../core/generic_hardware/RadioLink.h"

void USLI2025Payload::loopOnce(uint32_t runtime, uint32_t dt, double altitudeM, double velocityMS, double netAccelMSS, double orientationDeg, double temp, double batteryVoltage) {
    // Save the current temperature, battery voltage, and orientation
    updateGroundData(temp, batteryVoltage, orientationDeg);

    /**
     * Pre-flight state
     * Detect launch
     */
    if (m_flightState == PRE_FLIGHT) {
        if (altitudeM > m_takeoffThresholdM) {
            if (m_stateTimer == 0) {
                m_stateTimer = runtime + 1000;
            }
        } else {
            m_stateTimer = 0;
        }

        if (m_stateTimer != 0 && runtime > m_stateTimer) {
            m_hardware->getDebugStream()->print("takeoff");
            m_hardware->getDebugStream()->println();
            m_flightState = FLIGHT;
            m_stateTimer = runtime + (1000 * 60 * 3);

            m_liftoffTime = runtime;
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

        calculateSurvivability(runtime, netAccelMSS);

        updateLandingBuffers(altitudeM, velocityMS, netAccelMSS, runtime);

        if (runtime > m_stateTimer || checkLanded()) {
            if (runtime > m_stateTimer) {
                m_hardware->getDebugStream()->print("flight timed out");
                m_hardware->getDebugStream()->println();
            }
            if (checkLanded()) {
                m_hardware->getDebugStream()->print("landing detected");
                m_hardware->getDebugStream()->println();
            }
            m_hardware->getDebugStream()->print("landed");
            m_hardware->getDebugStream()->println();

            m_payloadData.time = (int32_t) (runtime - m_liftoffTime);
            m_payloadData.landVel = getLandingVelocity();
            m_payloadData.accel = getLandingAccelG();
            m_flightState = LANDED;


            m_stateTimer = runtime + (1000 * 60 * 4);
        }
    }

        /**
         * Landed state
         * Transmit data
         */
    else if (m_flightState == LANDED) {
        if (runtime > m_nextDeployTime) {
            m_hardware->getDebugStream()->print("deploy");
            m_hardware->getDebugStream()->println();
            m_nextDeployTime = runtime + 20000;
            if (m_transmitAllowed && runtime < m_stateTimer) {
                deployLegs();
                m_hardware->delay(2000);
            }
        }

        if (runtime > m_nextTransmitTime) {
            m_hardware->getDebugStream()->print("transmit");
            m_hardware->getDebugStream()->println();
            m_nextTransmitTime = runtime + 5000;
            if (m_transmitAllowed && runtime < m_stateTimer) {
                sendTransmission(runtime);
            }
        }
    }

        // Should never happen
    else {
        m_flightState = PRE_FLIGHT;
    }
}

void USLI2025Payload::updateGroundData(double temp, double batteryVoltage, double orientationRad) {
    m_payloadData.ort = (int32_t) orientationRad;
    m_payloadData.battery = (int32_t) double(batteryVoltage * 10.0);
    m_payloadData.temp = (int32_t) temp;
}

void USLI2025Payload::setup(HardwareAbstraction* hardware) {
    m_hardware = hardware;
}


void USLI2025Payload::deployLegs() const {
    if (m_hardware->getNumPyros() > 0) {
        m_hardware->getPyro(0)->fire();
        m_hardware->delay(250);
        m_hardware->getPyro(0)->disable();
    }
}

void USLI2025Payload::begin(const char* callsign) {
    m_transmitStringLocation = m_transmitBuffer;
    m_transmitStringLocation += sprintf(m_transmitStringLocation, "%s", callsign);
}

void USLI2025Payload::addStr(const char* str) {
    m_transmitStringLocation += sprintf(m_transmitStringLocation, ";%s", str);
}

void USLI2025Payload::addInt(int num) {
    m_transmitStringLocation += sprintf(m_transmitStringLocation, ";%d", num);
}

void USLI2025Payload::sendTransmission(uint32_t runtime) {
    if (m_hardware->getNumRadioLinks() > 0) {
        RadioLink* m_aprsModulation = m_hardware->getRadioLink(0);

        begin("KC1UAW");
        addInt((int) double(runtime / 1000.0));
        addInt((int) double((m_payloadData.time + 3000) / 1000.0));
        addInt(m_payloadData.temp);
        addInt(m_payloadData.battery);
        addInt(m_payloadData.alt);
        addInt(m_payloadData.ort);
        addInt(m_payloadData.maxVel);
        addInt(m_payloadData.landVel);
        addInt(m_payloadData.accel);
        addInt(m_payloadData.suviv);
        addStr("KC1UAW");

        m_hardware->getDebugStream()->print(m_transmitBuffer);
        m_hardware->getDebugStream()->println();
        m_aprsModulation->transmit((uint8_t*) m_transmitBuffer, 0);

        echoStringLora = true;
    }
}

const char* USLI2025Payload::getTransmitStr() {
    return m_transmitBuffer;
}


void USLI2025Payload::calculateSurvivability(uint32_t runTimeMs, double acceleration) {    // total time >= 10, 0 survivability and return

    // if acceleration is greater than the threshold at the current block
    if (acceleration > m_accelThreshold[m_survivabilityTotalTime]) {
        //set start time for acceleration over threshold if its not tracking
        if (m_survivabilityStartTime == -1) {
            m_survivabilityStartTime = (int) runTimeMs;
        }
        //total time currently over threshold in terms of seconds
        m_survivabilityTotalTime = (runTimeMs - m_survivabilityStartTime) / 1000;

        if (m_survivabilityTotalTime >= 10) {
            m_payloadData.suviv = 0;
            return;
        }

        double bounds = m_redLine[m_survivabilityTotalTime] - m_accelThreshold[m_survivabilityTotalTime];
        double newSurvivability = (1.0 - (double(acceleration - m_accelThreshold[m_survivabilityTotalTime]) / (bounds))) * 100;
        // only update if newSurvivability is lower than current one
        if (newSurvivability < m_payloadData.suviv) {
            if (newSurvivability < 0) {
                m_payloadData.suviv = 0;
            } else {
                m_payloadData.suviv = (int) newSurvivability;
            }
        }
    }
        //normal acceleration range
    else {
        m_survivabilityStartTime = -1;
    }
}


void USLI2025Payload::updateLandingBuffers(double altitude, double velocity, double acceleration, uint32_t timeMs) {
    bufferIndex++;
    if (bufferIndex >= BUFF_SIZE) bufferIndex = 0;

    altitudeBuff[bufferIndex] = altitude;
    timeMsBuff[bufferIndex] = timeMs;
    accelerationBuff[bufferIndex] = acceleration;
}

bool USLI2025Payload::checkLanded() {
    int minAlt = 9999999;
    int maxAlt = -999999;

    for (int i = 0; i < 20 * 3; i++) {
        int alt = altitudeBuff[(bufferIndex + BUFF_SIZE - i) % BUFF_SIZE];
        if (alt > maxAlt) {
            maxAlt = alt;
        }
        if (alt < minAlt) {
            minAlt = alt;
        }
    }


    if (abs(maxAlt - minAlt) < 3) {
        return true;
    }
    return false;
}

uint16_t USLI2025Payload::getLandingVelocity() {
    int preLandingIndex = (bufferIndex + BUFF_SIZE - 80) % BUFF_SIZE;
    int oldestIndex = (bufferIndex + BUFF_SIZE + 1) % BUFF_SIZE;

    int preLandingAltitude = altitudeBuff[preLandingIndex];
    uint32_t preLandingTime = timeMsBuff[preLandingIndex];

    int oldestAltitude = altitudeBuff[oldestIndex];
    uint32_t oldestTime = timeMsBuff[oldestIndex];

    double dt = preLandingTime - oldestTime;
    dt /= 1000;
    double dz = oldestAltitude - preLandingAltitude;

    return (int) round(dz / dt);
}

uint16_t USLI2025Payload::getLandingAccelG() {
    double max = 0;
    for (int i = 0; i < BUFF_SIZE; i++) {
        if (accelerationBuff[i] > max) {
            max = accelerationBuff[i];
        }
    }
    return round(max / 10.0);
}




