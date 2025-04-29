#include <Arduino.h>
#include <Adafruit_GPS.h>
#include "../drivers/arduino/UART_GPS.h"

#define GPS_SERIAL Serial1

UART_GPS gps(&GPS_SERIAL);

void setup() {
  
  delay(1000);  
  // Initialize and configure GPS
  gps.setup();
  
  delay(1000);
}

void loop() {
  // Read GPS data
  gps.read();

  // delay(50);
}
