#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UART_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UART_GPS_H

#include <Adafruit_GPS.h>
#include <GPS.h>


class UART_GPS final : public GPS {
public:
    explicit UART_GPS(HardwareSerial* serial);
    void setup() final;
    void read() final;
    bool checkDataFit() const;
    String getLastGPSData() const;

private:
    Adafruit_GPS* m_gps;
    String m_lastGPSData;
    static constexpr size_t MAX_GPS_BUFFER_SIZE = 512;
};

#endif




