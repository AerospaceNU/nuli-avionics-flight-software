//
// Created by chris on 11/18/2024.
//

#ifndef DESKTOP_CUSTOMCSVPARSER_H
#define DESKTOP_CUSTOMCSVPARSER_H


#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

typedef struct {
    uint32_t timestamp_s;
    uint32_t timestamp_ms;
    int16_t imu1_accel_x;
    int16_t imu1_accel_y;
    int16_t imu1_accel_z;
    int16_t imu1_gyro_x;
    int16_t imu1_gyro_y;
    int16_t imu1_gyro_z;
    int16_t imu1_mag_x;
    int16_t imu1_mag_y;
    int16_t imu1_mag_z;
    int16_t imu2_accel_x;
    int16_t imu2_accel_y;
    int16_t imu2_accel_z;
    int16_t imu2_gyro_x;
    int16_t imu2_gyro_y;
    int16_t imu2_gyro_z;
    int16_t imu2_mag_x;
    int16_t imu2_mag_y;
    int16_t imu2_mag_z;
    int16_t high_g_accel_x;
    int16_t high_g_accel_y;
    int16_t high_g_accel_z;
    double baro1_temp;
    double baro1_pres;
    double baro2_temp;
    double baro2_pres;
    double gps_lat;
    double gps_long;
    double gps_alt;
    double battery_voltage;
    uint8_t pyro_cont;  // @TODO: These values were zero on the provided CSV, assuming to be implemented
    uint8_t heading;
    uint8_t vtg;
    double pos_x;
    double pos_y;
    double pos_z;
    double vel_x;
    double vel_y;
    double vel_z;
    double q_x;
    double q_y;
    double q_z;
    double q_w;
    uint8_t state;
    double baro_pres_avg;
    double gps_lat_mod;
    double gps_long_mod;
    double imu1_accel_x_real;
    double imu1_accel_y_real;
    double imu1_accel_z_real;
    double imu2_accel_x_real;
    double imu2_accel_y_real;
    double imu2_accel_z_real;
    double imu1_gyro_x_real;
    double imu1_gyro_y_real;
    double imu1_gyro_z_real;
    double imu2_gyro_x_real;
    double imu2_gyro_y_real;
    double imu2_gyro_z_real;
    double imu1_mag_x_real;
    double imu1_mag_y_real;
    double imu1_mag_z_real;
    double imu2_mag_x_real;
    double imu2_mag_y_real;
    double imu2_mag_z_real;
    double imu_accel_x_avg;
    double imu_accel_y_avg;
    double imu_accel_z_avg;
    double imu_gyro_x_avg;
    double imu_gyro_y_avg;
    double imu_gyro_z_avg;
    double imu_mag_x_avg;
    double imu_mag_y_avg;
    double imu_mag_z_avg;
    double high_g_accel_x_real;
    double high_g_accel_y_real;
    double high_g_accel_z_real;
    double baro_temp_avg;
} aeroCsvRow_s;

/**
 *
 */
/**
 * @class CustomCsvParser
 * @brief
 * @details This class presupposes the structure of the input CSV
 * @param
 */
class CustomCsvParser {
public:
    using CSVValue = std::variant<int, double, std::string>;

    CustomCsvParser() = default;

    explicit CustomCsvParser(std::string fileName);

    int8_t parse(const std::string &filename, bool header);


    aeroCsvRow_s convertRow(const std::map<std::string, std::vector<std::string>> &csvData, size_t index);

    std::vector<aeroCsvRow_s> m_csv;

    int8_t getRow(size_t index, aeroCsvRow_s &row);

    size_t getSize() const;

private:
    aeroCsvRow_s convertRow(std::string &line);

    template<typename T>
    T convert(const std::string &value);

    static std::vector<std::string> splitCSV(const std::string &line);

    std::vector<std::string> m_headers;

    const std::string m_fileName;
};


#endif //DESKTOP_CUSTOMCSVPARSER_H
