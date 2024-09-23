#include <iostream>

int main() {
    volatile int a;
    std::cout << a << std::endl;
}


/**
* Notes:
 * - States should be their own thing class?
 * - Sensor data:
 *      -
 *
 * Abstract Sensor Class:
 *  - Raw data X #ofSensors
 *  - Calculated data (degs, alt, pos)
 *  Further Abstracted Sensor Class????
 * - Combined data (1XSensor)
 * Processing Class:
 *  - Filtered data
 *  - Outputs struct or API
 *
*/