#include "Avionics.h"
#include <Arduino.h>
#include "boards/SillyGoosePins.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "drivers/arduino/IndicatorLED.h"
#include "drivers/arduino/IndicatorBuzzer.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/altitude_kf.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/Parser.h"
#include "core/Logger.h"
#include "core/AvionicsCore.h"
#include "core/Configuration.h"
#include "core/altitude_kf.h"
#include "core/StateMachine.h"

ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);

MS5607Sensor barometer;
ICM20602Sensor icm20602;
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);

S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);

IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000);

AltitudeKf altitudeKf;
StateMachine stateMachine;

ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(false);
ConfigurationID_e configSet[] = {DROGUE_DELAY, MAIN_ELEVATION};
Configuration configuration({configSet, Configuration::REQUIRED_CONFIGS});
HardwareAbstraction hardware;

void setup() {
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    Serial.begin(9600);
    // while (!Serial);
    Serial.println("Starting");
    // System
    hardware.setLoopRate(100);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    hardware.setConfigurationMemory(&fram);
    hardware.setConfiguration(&configuration);
    // Devices
    hardware.addPyro(&droguePyro);
    hardware.addPyro(&mainPyro);
    hardware.addVoltageSensor(&batteryVoltageSensor);
    hardware.addBarometer(&barometer);
    hardware.addGenericSensor(&icm20602);
    hardware.addAccelerometer(icm20602.getAccelerometer());
    hardware.addGyroscope(icm20602.getGyroscope());
    hardware.addFlashMemory(&flash);
    hardware.addIndicator(&led);
    hardware.addIndicator(&buzzer);
    hardware.setup();

    stateMachine.setup();
    altitudeKf.calculateDiscreteTimeA(.01);

}


void loop() {
    const uint32_t start = micros();

    hardware.enforceLoopTime();
    hardware.readAllSensors();

    for (int i = 0; i < hardware.getNumIndicators(); i++) {
        if ((hardware.getLoopTimestampMs() / 250) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }

    altitudeKf.predict(hardware.getLoopTimestampMs() / 1000.0);
    altitudeKf.dataUpdate(barometer.getAltitudeM(), icm20602.getAccelerometer()->getAccelerationsMSS().z);

    configuration.pushUpdates();

    const uint32_t end = micros();
    Serial.print(altitudeKf.getAltitude());
    Serial.print('\t');
    Serial.print(hardware.getLastTickDuration());
    Serial.print('\t');
    Serial.print((end - start) / 1000.0);
    Serial.print('\t');
    Serial.print(batteryVoltageSensor.getVoltage());
    Serial.print('\t');
    Serial.println(barometer.getAltitudeM());
}
