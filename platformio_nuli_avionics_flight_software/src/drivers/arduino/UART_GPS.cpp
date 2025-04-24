#include "UART_GPS.h"
// #define DEBUG_SERIAL Serial

UART_GPS::UART_GPS(HardwareSerial* serial):
    GPS(),
    m_gps(new Adafruit_GPS(serial))
    , m_lastGPSData("") 
    {
}

void UART_GPS::setup() {
    m_gps->begin(9600);

    // m_gps->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

    delay(1000);

}


void UART_GPS::read() {
    m_gps->read();  // Read data from GPS module
    Serial.println("read recipient");
    //delay(2000);


    
    if (m_gps->newNMEAreceived() || true) {
        m_lastGPSData = m_gps->lastNMEA();  // Store last NMEA sentence
        Serial.print("Raw GPS data: ");
        Serial.println(m_lastGPSData);
        
       
            char buffer[m_lastGPSData.length() + 1];
            strcpy(buffer, m_lastGPSData.c_str());
    
            if (m_gps->parse(buffer)) {
                // Parse successful, update class variables
                latitude = m_gps->latitudeDegrees;
                longitude = m_gps->longitudeDegrees;
                altitude = m_gps->altitude;
                fixQuality = m_gps->fixquality;
                satsTracked = m_gps->satellites;
                hdop = m_gps->HDOP;
    
                // Debug output
                Serial.print("Lat: "); Serial.println(latitude, 6);
                Serial.print("Lon: "); Serial.println(longitude, 6);
                Serial.print("Alt: "); Serial.println(altitude);
                Serial.print("Fix Quality: "); Serial.println(fixQuality);
                Serial.print("Satellites: "); Serial.println(satsTracked);
                Serial.print("HDOP: "); Serial.println(hdop);
            } else {
                Serial.println("GPS parse failed!");
            }
        
    }    
}


bool UART_GPS::checkDataFit() const {
    return m_lastGPSData.length() <= MAX_GPS_BUFFER_SIZE;
}

String UART_GPS::getLastGPSData() const {
    return m_lastGPSData;
}
