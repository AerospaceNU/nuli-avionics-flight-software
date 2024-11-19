/*
#include "UART_GPS.h"


UART_GPS::UART_GPS(HardwareSerial* gpsSerial) : 
    GPS(),  // Call base class constructor
    m_gps(gpsSerial) {}




void UART_GPS::setup() {
    GPS::setup();
    m_gps.begin(9600); // Set to your GPS baud rate
    m_gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    m_gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}




void UART_GPS::read() {

    // Process the data if we have a complete sentence

    if (m_gps.newNMEAreceived() && m_gps.parse(m_gps.lastNMEA())) {

        // Update the parent class protected members

        latitude = m_gps.latitudeDegrees;

        longitude = m_gps.longitudeDegrees;

        altitude = m_gps.altitude;

        fixQuality = m_gps.fixquality;

        satsTracked = m_gps.satellites;

        hdop = m_gps.HDOP;

    }

}




/*
void UART_GPS::read() {
    m_lastGPSData = "";
    
    // Clear old data
    while (m_gps.available()) {
        m_gps.read();
    }

    // Read new data
    unsigned long startTime = millis();
    do {
        while (m_gps.available()) {
            char c = m_gps.read();
            m_lastGPSData += c;
            if (c == '\n') {
                // We have a complete NMEA sentence
                break;
            }
        }
        // Timeout after 1 second
        if (millis() - startTime > 1000) break;
    } while (m_lastGPSData.length() < MAX_GPS_BUFFER_SIZE);

    // Process the data if we have a complete sentence
    if (m_gps.newNMEAreceived()) {
        m_gps.parse(m_gps.lastNMEA());
    }
}
//initial comment break
bool UART_GPS::checkDataFit() const {
    return m_lastGPSData.length() <= MAX_GPS_BUFFER_SIZE;
}

String UART_GPS::getLastGPSData() const {
    return m_lastGPSData;
}
*/



#include "UART_GPS.h"

UART_GPS::UART_GPS(HardwareSerial* gpsSerial)
    : GPS(), m_gps(gpsSerial) {}

void UART_GPS::setup() {
    constexpr int DEFAULT_BAUD_RATE = 9600; // Use a named constant
    m_gps.begin(DEFAULT_BAUD_RATE);
    Serial.println("helooo");
    // Configure NMEA sentence output and update rate
    m_gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    m_gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

void UART_GPS::read() {
    // Check if new data is available and parse it
    if (m_gps.newNMEAreceived()) {
        if (m_gps.parse(m_gps.lastNMEA())) {
            // Update parent class attributes
            latitude = m_gps.latitudeDegrees;
            longitude = m_gps.longitudeDegrees;
            altitude = m_gps.altitude;
            fixQuality = m_gps.fixquality;
            satsTracked = m_gps.satellites;
            hdop = m_gps.HDOP;

            // Store the last received sentence
            m_lastGPSData = m_gps.lastNMEA();
        } else {
            // Handle parse failure 
            Serial.println("error parsing");
        }
    }
}

bool UART_GPS::checkDataFit() const {
    return m_lastGPSData.length() <= MAX_GPS_BUFFER_SIZE;
}

String UART_GPS::getLastGPSData() const {
    return m_lastGPSData;
}

