#include <Arduino.h>
#include "Avionics.h"
#include "boards/SillyGoosePins.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/Parser.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/Logger.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "core/AvionicsCore.h"
#include "core/Configuration.h"

Logger logger;

const ConfigurationID_e set3[] = {GROUND_ELEVATION, GROUND_TEMPERATURE, DODAD_THING};
const ConfigSet_s allConfigs[] = {Configuration::REQUIRED_CONFIGS, Logger::REQUIRED_CONFIGS, set3};
Configuration config(allConfigs);

void example() {

    ConfigurationData<double>* radioFrequency = config.getConfigurable<RADIO_FREQUENCY>();
    ConfigurationData<uint32_t>* configVersion = config.getConfigurable<CONFIGURATION_VERSION>();
    ConfigurationData<TestStruct_s>* testData = config.getConfigurable<DODAD_THING>();

    radioFrequency->set(915.3);
    radioFrequency->get();
    configVersion->set(3);
    configVersion->get();
    testData->set({2, 3, 1});
    testData->get()->a;
    testData->get()->b;
    testData->get()->c;
}

ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(false);

ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);

MS5607Sensor barometer;
ICM20602Sensor icm20602Sensor;
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);

S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);

//Configuration configuration;
Filters filter;
HardwareAbstraction hardware;
AvionicsCore avionicsCore;

void callback_name(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    Serial.println("erasing");
    flash.eraseAll();
    Serial.println("done");
}

void fireDrogue(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    Serial.println("drogue");
    droguePyro.fire();
    delay(1000);
    droguePyro.disable();
}

void fireMain(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    Serial.println("main");
    mainPyro.fire();
    delay(1000);
    mainPyro.disable();
}

Parser myParser;
SimpleFlag stopTransmit("--erase", "erases", true, 255, callback_name);
BaseFlag* stopTransmitGroup[] = {&stopTransmit};
SimpleFlag fireMsg("--fireDrogue", "erases", true, 255, fireDrogue);
BaseFlag* fireMsgTransmitGroup[] = {&fireMsg};
SimpleFlag fire2Msg("--fireMain", "erases", true, 255, fireMain);
BaseFlag* fir2eMsgTransmitGroup[] = {&fire2Msg};

void getSerialInput(char* buffer) {
    if (!Serial.available()) {
        buffer[0] = '\0';
        return;
    }

    size_t bytesRead = Serial.readBytes(buffer, 254);
    buffer[bytesRead] = '\0';

    // clear serial buffer
    while (Serial.available()) {
        Serial.read();  // Discard any remaining bytes
    }
}

void serialSender() {
    char buf[255];  // @TODO: change into a macro/const
    getSerialInput(buf);

    // Only parse if we actually received something
    if (buf[0] != '\0') {
        Serial.print("Parsing input: ");
        Serial.println(buf);
        myParser.parse(buf);
    }
}

void setup() {
    pinMode(FRAM_CS_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);
    digitalWrite(FRAM_CS_PIN, HIGH);
    SPI.begin();

    Serial.begin(9600);
    while (!Serial);

    myParser.addFlagGroup(stopTransmitGroup);
    myParser.addFlagGroup(fireMsgTransmitGroup);
    myParser.addFlagGroup(fir2eMsgTransmitGroup);

    hardware.setLoopRate(100);
    // System configuration--erase
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    // Devices
    hardware.addPyro(&droguePyro);
    hardware.addPyro(&mainPyro);
    hardware.addVoltageSensor(&batteryVoltageSensor);
    hardware.addBarometer(&barometer);
    hardware.addGenericSensor(&icm20602Sensor);
    hardware.addAccelerometer(icm20602Sensor.getAccelerometer());
    hardware.addGyroscope(icm20602Sensor.getGyroscope());
    hardware.addFlashMemory(&flash);
    hardware.addFramMemory(&fram);

    hardware.setup();

}

bool flag = false;

void loop() {
    hardware.enforceLoopTime();
    hardware.readAllSensors();

    digitalWrite(LIGHT_PIN, ((hardware.getLoopTimestampMs() / 200) % 2 == 0) ? LOW : HIGH);
    if (((hardware.getLoopTimestampMs() / 1000) % 2 == 0)) {
        if (flag) {
            tone(BUZZER_PIN, 4000, 1000);
            flag = false;
        }
    } else {
        flag = true;
    }

    serialSender();
    myParser.runFlags();
    myParser.resetFlags();


    Serial.println(hardware.getLoopDtMs());
}

