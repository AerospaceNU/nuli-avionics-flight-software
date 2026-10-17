#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM42605Sensor_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM42605Sensor_H

#include <Avionics.h>
#include "core/generic_hardware/GenericHardware.h"
#include "core/transform/Vector3DTransform.h"


/** @brief ICM42605 IMU driver (I2C) */
class ICM42605Sensor final : public GenericSensor {
public:
    explicit ICM42605Sensor(const Vector3DTransform *transform);

    void setup(DebugStream* debugStream) override;

    void read() override;

    Accelerometer* getAccelerometer();

    Gyroscope* getGyroscope();

private:
    void readAccelAndGyroBatch();

    Accelerometer m_accelerometer;
    Gyroscope m_gyroscope;

    int16_t accelData[3] = {};
    int16_t gyroData[3] = {};
    int16_t tempRaw = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_ICM42605Sensor_H
