
#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UART_GPS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UART_GPS_H

#include <GPS.h>
#include <Avionics.h>
#include <GenericSensor.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>

/**
 * @class UART_GPS
 * @brief Driver for GPS module using UART
 * @details Uses the Adafruit GPS library to read GPS data
 */
class UART_GPS final : public GPS {
public:
    /**
     * @brief Creates a UART_GPS object
     * @param gpsSerial HardwareSerial object for GPS communication
     */ 
    //explicit UART_GPS(HardwareSerial* gpsSerial): GPS(), m_gps(gpsSerial) {}
    explicit UART_GPS(HardwareSerial* gpsSerial);

    /**
     * @brief Initializes the GPS module
     */
    void setup() final;

    /**
     * @brief Reads data from the GPS module
     */
    void read() final;

    /**
     * @brief Checks if the received GPS data fits within the buffer
     * @return true if data fits, false if it exceeds buffer size
     */
    bool checkDataFit() const;

    /**
     * @brief Gets the last received GPS data
     * @return String containing the last GPS data
     */
    String getLastGPSData() const;

private:    
    Adafruit_GPS m_gps;
    String m_lastGPSData;
    static constexpr size_t MAX_GPS_BUFFER_SIZE = 512; // Adjust as needed
};

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_UART_GPS_H