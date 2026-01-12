#include "LSM6DSV320X.h"
#define CTRL1_XL 0x10
LSM6DSV320X imu(10);

void setup() {
  Serial.begin(115200);
  delay(2000);
  imu.begin();

  if (!imu.testConnection()) {
    Serial.println("LSM IMU not found!");
    while(1);
  }
  else {
    Serial.println("LSM IMU Found! :))");
  }
  imu.configureSensor();
}

void loop() {
    
  int16_t lax, lay, laz, hax, hay, haz, gx, gy, gz;
  imu.readLowGAccel(lax, lay, laz);
  imu.readHighGAccel(hax, hay, haz);
  imu.readGyro(gx, gy, gz);

  Serial.print("Low G Accel: ");
  Serial.print(lax); Serial.print(", ");
  Serial.print(lay); Serial.print(", ");
  Serial.println(laz);
  
  Serial.print("High G Accel: "); 
  Serial.print(hax); Serial.print(", ");
  Serial.print(hay); Serial.print(", ");
  Serial.println(haz);

  Serial.print("Gyro: ");
  Serial.print(gx); Serial.print(", ");
  Serial.print(gy); Serial.print(", ");
  Serial.println(gz);

  delay(200);
}
