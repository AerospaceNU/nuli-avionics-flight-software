#ifndef ALTITUDE_BEEPS_H
#define ALTITUDE_BEEPS_H

#include "Avionics.h"
#include "core/generic_hardware/Indicator.h"
#include "core/HardwareAbstraction.h"

enum BeepState {
   IDLE,
   BEEPING,
   PAUSE_BETWEEN_BEEPS,
   PAUSE_BETWEEN_DIGITS,
   DONE
};

class AltitudeBeeper {
private:
   HardwareAbstraction* m_hardware = nullptr;
   BeepState state = IDLE;
   uint32_t lastActionTime = 0;
   int currentAltitude = 0;
   int currentDigitIndex = 0;
   int currentBeepCount = 0;
   int digits[4] = {0}; // Max 4 digits for altitude (0-9999m)
   int numDigits = 0;
   
   // Timing constants (adjust these as needed)
   const uint32_t beepDuration = 150;        // ms per beep
   const uint32_t pauseBetweenDuration = 200;  // ms between beeps in same digit
   const uint32_t pauseBetweenDigits = 500; // ms between different digits
   const uint32_t repeatInterval = 5000;     // ms before re-announcing altitude
   
   
   void extractDigits(int altitude) {
      numDigits = 0;
      if (altitude == 0) {
         digits[0] = 0;
         numDigits = 1;
         return;
      }
      
      int temp = altitude;
      while (temp > 0 && numDigits < 4) {
         digits[numDigits++] = temp % 10;
         temp /= 10;
      }
      
      // Reverse to get most significant digit first
      for (int i = 0; i < numDigits / 2; i++) {
         int swap = digits[i];
         digits[i] = digits[numDigits - 1 - i];
         digits[numDigits - 1 - i] = swap;
      }
   }
   
public:
   void start(int altitude) {
      currentAltitude = altitude;
      extractDigits(altitude);
      state = BEEPING;
      currentDigitIndex = 0;
      currentBeepCount = 0;
   }
   
   void update(uint32_t currentTime) {
      switch (state) {
         case IDLE:
            // Waiting to start
            break;
               
         case BEEPING:
            for(int i=0; i<m_hardware->getNumIndicators(); i++){
               m_hardware->getIndicator(i)->on();
            }
            m_hardware->getIndicator(1)->on();
            if (currentTime - lastActionTime >= beepDuration) {
               for(int i=0; i<m_hardware->getNumIndicators(); i++){
                  m_hardware->getIndicator(i)->off(); // Turn off buzzer after beep duration
               }
               currentBeepCount++;
               
               int targetBeeps = digits[currentDigitIndex];
               if (targetBeeps == 0){
                  targetBeeps = 10; // 0 = 10 beeps
               } 
               
               if (currentBeepCount >= targetBeeps) {
                  // Finished this digit
                  currentDigitIndex++;
                  currentBeepCount = 0;
                  
                  if (currentDigitIndex >= numDigits) {
                        state = DONE;
                  } else {
                        state = PAUSE_BETWEEN_DIGITS;
                  }
                  lastActionTime = currentTime;
               } else {
                  state = PAUSE_BETWEEN_BEEPS;
                  lastActionTime = currentTime;
               }
            }
            break;
               
         case PAUSE_BETWEEN_BEEPS:
            if (currentTime - lastActionTime >= pauseBetweenDuration) {
               for(int i=0; i<m_hardware->getNumIndicators(); i++){
                  m_hardware->getIndicator(i)->on(); // Start next beep
               }
               state = BEEPING;
               lastActionTime = currentTime;
            }
            break;
               
         case PAUSE_BETWEEN_DIGITS:
            if (currentTime - lastActionTime >= pauseBetweenDigits) {
               for(int i=0; i<m_hardware->getNumIndicators(); i++){
                  m_hardware->getIndicator(i)->on(); // Start first beep of next digit
               }
               state = BEEPING;
               lastActionTime = currentTime;
            }
            break;
               
         case DONE:
            // Wait before repeating (optional)
            if (currentTime - lastActionTime >= repeatInterval) {
               start(currentAltitude); // Restart announcement
            }
            break;
      }
   }
   
   bool isActive() {
      return state != IDLE && state != DONE;
   }
};

#endif // ALTITUDE_BEEPS_H