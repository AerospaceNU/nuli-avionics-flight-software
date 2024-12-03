//
// Created by chris on 11/18/2024.
//

#include "CustomCsvParser.h"
#include <fstream>
#include <sstream>

int8_t CustomCsvParser::parse(const std::string &filename, const bool header = false) {
    // check that the file ends with .csv
    if (!filename.ends_with(".csv")) {
        perror("Invalid File (not a csv file)");
        return -1;
    }

    // open the file
    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        perror("Unable to open file");
        return -1;
    }

    // if header, skip line
    std::string line;
    if (header) {
        std::getline(inputFile, line);
    }

    while(std::getline(inputFile, line)) {
        FlightDataRow_s row = convertRow(line);
        m_csv.push_back(row);
    }

    inputFile.close();

    return 0;
}

// Converts a string into type T
template<typename T> T CustomCsvParser::convertToTypeT(const std::string &value) {
    std::istringstream iss(value);
    T result;
    iss >> result;
    if (iss.fail()) {
        throw std::runtime_error("Conversion error for value: " + value);
    }
    return result;
}

// Function to split CSV string
std::vector<std::string> CustomCsvParser::splitCSV(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;

    while (std::getline(stream, token, ',')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        tokens.push_back(token);
    }
    return tokens;
}

// converts a row to a struct
CustomCsvParser::FlightDataRow_s CustomCsvParser::convertRow(std::string &line) {
    std::vector<std::string> tokens = splitCSV(line);
    FlightDataRow_s row;
    row.timestamp_s = convertToTypeT<uint32_t>(tokens[0]);
    row.timestamp_ms = convertToTypeT<uint32_t>(tokens[1]);

    row.imu1_accel_s.x = convertToTypeT<double>(tokens[2]);
    row.imu1_accel_s.y = convertToTypeT<double>(tokens[3]);
    row.imu1_accel_s.z = convertToTypeT<double>(tokens[4]);

    row.imu1_gyro_s.x = convertToTypeT<double>(tokens[5]);
    row.imu1_gyro_s.y = convertToTypeT<double>(tokens[6]);
    row.imu1_gyro_s.z = convertToTypeT<double>(tokens[7]);

    row.imu1_mag_s.x = convertToTypeT<int16_t>(tokens[8]);
    row.imu1_mag_s.y = convertToTypeT<int16_t>(tokens[9]);
    row.imu1_mag_s.z = convertToTypeT<int16_t>(tokens[10]);

    row.imu2_accel_s.x = convertToTypeT<int16_t>(tokens[11]);
    row.imu2_accel_s.y = convertToTypeT<int16_t>(tokens[12]);
    row.imu2_accel_s.z = convertToTypeT<int16_t>(tokens[13]);

    row.imu2_gyro_s.x = convertToTypeT<int16_t>(tokens[14]);
    row.imu2_gyro_s.y = convertToTypeT<int16_t>(tokens[15]);
    row.imu2_gyro_s.z = convertToTypeT<int16_t>(tokens[16]);

    row.imu2_mag_s.x = convertToTypeT<int16_t>(tokens[17]);
    row.imu2_mag_s.y = convertToTypeT<int16_t>(tokens[18]);
    row.imu2_mag_s.z = convertToTypeT<int16_t>(tokens[19]);

    row.high_g_accel_s.x = convertToTypeT<int16_t>(tokens[20]);
    row.high_g_accel_s.y = convertToTypeT<int16_t>(tokens[21]);
    row.high_g_accel_s.z = convertToTypeT<int16_t>(tokens[22]);

    row.baro1_s.temp = convertToTypeT<double>(tokens[23]);
    row.baro1_s.pres = convertToTypeT<double>(tokens[24]);

    row.baro2_s.temp = convertToTypeT<double>(tokens[25]);
    row.baro2_s.pres = convertToTypeT<double>(tokens[26]);

    row.gps_s.latitude = convertToTypeT<double>(tokens[27]);
    row.gps_s.longitude = convertToTypeT<double>(tokens[28]);
    row.gps_s.alt = convertToTypeT<double>(tokens[29]);

    row.battery_voltage = convertToTypeT<double>(tokens[30]);
    row.pyro_cont = convertToTypeT<uint8_t>(tokens[31]);
    row.heading = convertToTypeT<uint8_t>(tokens[32]);
    row.vtg = convertToTypeT<uint8_t>(tokens[33]);

    row.pos_s.x = convertToTypeT<double>(tokens[34]);
    row.pos_s.y = convertToTypeT<double>(tokens[35]);
    row.pos_s.z = convertToTypeT<double>(tokens[36]);

    row.vel_s.x = convertToTypeT<double>(tokens[37]);
    row.vel_s.y = convertToTypeT<double>(tokens[38]);
    row.vel_s.z = convertToTypeT<double>(tokens[39]);

    row.q_s.x = convertToTypeT<double>(tokens[40]);
    row.q_s.y = convertToTypeT<double>(tokens[41]);
    row.q_s.z = convertToTypeT<double>(tokens[42]);
    row.q_s.w = convertToTypeT<double>(tokens[43]);

    row.state = convertToTypeT<uint8_t>(tokens[44]);
    row.baro_pres_avg = convertToTypeT<double>(tokens[45]);

    row.gps_mod_s.latitude = convertToTypeT<double>(tokens[46]);
    row.gps_mod_s.longitude = convertToTypeT<double>(tokens[47]);

    row.imu1_accel_real_s.x = convertToTypeT<double>(tokens[48]);
    row.imu1_accel_real_s.y = convertToTypeT<double>(tokens[49]);
    row.imu1_accel_real_s.z = convertToTypeT<double>(tokens[50]);

    row.imu2_accel_real_s.x = convertToTypeT<double>(tokens[51]);
    row.imu2_accel_real_s.y = convertToTypeT<double>(tokens[52]);
    row.imu2_accel_real_s.z = convertToTypeT<double>(tokens[53]);

    row.imu1_gyro_real_s.x = convertToTypeT<double>(tokens[54]);
    row.imu1_gyro_real_s.y = convertToTypeT<double>(tokens[55]);
    row.imu1_gyro_real_s.z = convertToTypeT<double>(tokens[56]);

    row.imu2_gyro_real_s.x = convertToTypeT<double>(tokens[57]);
    row.imu2_gyro_real_s.y = convertToTypeT<double>(tokens[58]);
    row.imu2_gyro_real_s.z = convertToTypeT<double>(tokens[59]);

    row.imu1_mag_real_s.x = convertToTypeT<double>(tokens[60]);
    row.imu1_mag_real_s.y = convertToTypeT<double>(tokens[61]);
    row.imu1_mag_real_s.z = convertToTypeT<double>(tokens[62]);

    row.imu2_mag_real_s.x = convertToTypeT<double>(tokens[63]);
    row.imu2_mag_real_s.y = convertToTypeT<double>(tokens[64]);
    row.imu2_mag_real_s.z = convertToTypeT<double>(tokens[65]);

    row.imu_accel_avg_s.x = convertToTypeT<double>(tokens[66]);
    row.imu_accel_avg_s.y = convertToTypeT<double>(tokens[67]);
    row.imu_accel_avg_s.z = convertToTypeT<double>(tokens[68]);

    row.imu_gyro_avg_s.x = convertToTypeT<double>(tokens[69]);
    row.imu_gyro_avg_s.y = convertToTypeT<double>(tokens[70]);
    row.imu_gyro_avg_s.z = convertToTypeT<double>(tokens[71]);

    row.imu_mag_avg_s.x = convertToTypeT<double>(tokens[72]);
    row.imu_mag_avg_s.y = convertToTypeT<double>(tokens[73]);
    row.imu_mag_avg_s.z = convertToTypeT<double>(tokens[74]);

    row.high_g_accel_real_s.x = convertToTypeT<double>(tokens[75]);
    row.high_g_accel_real_s.y = convertToTypeT<double>(tokens[76]);
    row.high_g_accel_real_s.z = convertToTypeT<double>(tokens[77]);

    row.baro_temp_avg = convertToTypeT<double>(tokens[78]);

    return row;
}

// Getters
CustomCsvParser::FlightDataRow_s CustomCsvParser::getRow(size_t index) const {
    return this->m_csv.at(index);
}

size_t CustomCsvParser::getSize() const {
    return this->m_csv.size();
}

