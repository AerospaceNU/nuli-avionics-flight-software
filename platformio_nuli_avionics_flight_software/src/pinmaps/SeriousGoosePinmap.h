#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SILLYGOOSEPINS_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SILLYGOOSEPINS_H

#define SENSE_R1 47.0f
#define SENSE_R2 10.0f
#define SILLY_GOOSE_NAME "SeriousGooseV1"

#define FRAM_CS_PIN (34)
#define FLASH_CS_PIN (12)
// #define FRAM_HOLD_PIN (PB06)
// #define FLASH_HOLD_PIN (PB07)
// #define IMU_INT_PIN (PB05)
// #define STATUS_PIN (PB04)
// #define BOOT_PIN (PA15)

#define RADIO_CS_PIN (38)
#define RADIO_BUSY_PIN (35)
#define RADIO_DIO1_PIN (10)
#define RADIO_TX_EN_PIN (39)
#define RADIO_RX_EN_PIN (37)
#define RADIO_RESET_PIN (36)

#define AUX_1_PIN (15)
#define AUX_2_PIN (18)
#define AUX_3_PIN (14)

#define BUZZER_PIN (4)
#define LIGHT_PIN (5)

#define VOLTAGE_SENSE_PIN (A3)
#define VOLTAGE_SENSE_SCALE (((SENSE_R1 + SENSE_R2) / SENSE_R2) * (3.3f / 1023.0f))

#define PYRO1_GATE_PIN (6)      // Main
#define PYRO1_SENSE_PIN (A6)    // Main
#define PYRO2_GATE_PIN (9)     // Drogue
#define PYRO2_SENSE_PIN (A2)    // Drogue
#define PYRO3_GATE_PIN (11)     // Aux
#define PYRO3_SENSE_PIN (A5)    // Aux
#define PYRO_SENSE_THRESHOLD (200)

#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SILLYGOOSEPINS_H
