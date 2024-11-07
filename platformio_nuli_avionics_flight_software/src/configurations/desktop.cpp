//#include <cmath>
//#include <cstring>
//
//#include "../drivers/samd21/AprsModulation.h"
//
//int main() {
//    AprsModulation aprsModulation(1);
////    aprsModulation.setup()
//    const char* str = "asdf345";
//    aprsModulation.transmit(str, strlen(str));
//    std::cout << "\n";
//}

//const double ATMOSPHERIC_PRESSURE_PA = 101325;
//const double GRAVITATIONAL_CONSTANT_M_SS = 9.80665;
//const double LAPSE_RATE_CONSTANT_K_M = 0.0065;
//const double GAS_CONSTANT_J_KG_K = 287.0474909;
//
//double calculateAltitude(double m_temperatureK, double m_pressurePa) {
//    return (m_temperatureK / LAPSE_RATE_CONSTANT_K_M) *
//           (1 - pow(m_pressurePa / ATMOSPHERIC_PRESSURE_PA, (GAS_CONSTANT_J_KG_K * LAPSE_RATE_CONSTANT_K_M) / GRAVITATIONAL_CONSTANT_M_SS));
//
//}
//
//double calculateAltitude2(double m_temperatureK, double m_pressurePa) {
//    // @todo add
//    return (m_temperatureK / -LAPSE_RATE_CONSTANT_K_M) *
//           (pow(m_pressurePa / ATMOSPHERIC_PRESSURE_PA, -(GAS_CONSTANT_J_KG_K * -LAPSE_RATE_CONSTANT_K_M) / GRAVITATIONAL_CONSTANT_M_SS) - 1);
//}
//
//int main() {
//    for(double a = ATMOSPHERIC_PRESSURE_PA; a > ATMOSPHERIC_PRESSURE_PA * .6; a -= 10) {
//        std::cout << calculateAltitude(293, a) << " : " << calculateAltitude2(293, a) << "\n";
//    }
//}


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