#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H

#include <Avionics.h>
#include "GenericSensor.h"

class GPS : public GenericSensor {
public:
//    explicit GPS(uint32_t baudRate) {
//        m_baudRate = baudRate;
//    }
//
//    void setup() override {
//        if(m_gpsSerial == nullptr) {
//            m_gpsSerial = &Serial1;
//        }
//        m_gpsSerial->begin(m_baudRate);
//    }
//
//    void read() override {
//        while (m_gpsSerial->available()) {
//            m_gpsSerial->read();
//        }
//    }

protected:
//    float latitude;
//    float longitude;
//    float altitude;
//    uint8_t fixQuality;
//    uint8_t satsTracked;
//    float hdop;

//    HardwareSerial *m_gpsSerial = nullptr;
    uint32_t m_baudRate;
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_GPS_H
