//
// Created by chris on 11/18/2024.
//

#include "CustomCsvParser.h"
#include <fstream>
#include <sstream>
#include <utility>

CustomCsvParser::CustomCsvParser(std::string fileName) : m_fileName(std::move(fileName)) {

}

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
        aeroCsvRow_s row = convertRow(line);
        m_csv.push_back(row);
    }

    inputFile.close();

    return 0;
}

// Converts a string into type T
template<typename T> T CustomCsvParser::convert(const std::string &value) {
    std::istringstream iss(value);
    T result;
    iss >> result;
    if (iss.fail()) {
        throw std::runtime_error("Conversion error for value: " + value);
    }
    return result;
}

aeroCsvRow_s CustomCsvParser::convertRow(const std::map<std::string, std::vector<std::string>> &csvData, size_t index) {
    aeroCsvRow_s row;
    row.timestamp_s = convert<uint32_t>(csvData.at("timestamp_s")[index]);
    row.timestamp_ms = convert<uint32_t>(csvData.at("timestamp_ms")[index]);
    row.imu1_accel_x = convert<int16_t>(csvData.at("imu1_accel_x")[index]);
    row.imu1_accel_y = convert<int16_t>(csvData.at("imu1_accel_y")[index]);
    row.imu1_accel_z = convert<int16_t>(csvData.at("imu1_accel_z")[index]);
    row.imu1_gyro_x = convert<int16_t>(csvData.at("imu1_gyro_x")[index]);
    row.imu1_gyro_y = convert<int16_t>(csvData.at("imu1.gyro_y")[index]);
    row.imu1_gyro_z = convert<int16_t>(csvData.at("imu1_gyro_z")[index]);
    row.imu1_mag_x = convert<int16_t>(csvData.at("imu1_mag_x")[index]);
    row.imu1_mag_y = convert<int16_t>(csvData.at("imu1_mag_y")[index]);
    row.imu1_mag_z = convert<int16_t>(csvData.at("imu1_mag_z")[index]);
    row.imu2_accel_x = convert<int16_t>(csvData.at("imu2_accel_x")[index]);
    row.imu2_accel_y = convert<int16_t>(csvData.at("imu2_accel_y")[index]);
    row.imu2_accel_z = convert<int16_t>(csvData.at("imu2_accel_z")[index]);
    row.imu2_gyro_x = convert<int16_t>(csvData.at("imu2_gyro_x")[index]);
    row.imu2_gyro_y = convert<int16_t>(csvData.at("imu2_gyro_y")[index]);
    row.imu2_gyro_z = convert<int16_t>(csvData.at("imu2_gyro_z")[index]);
    row.imu2_mag_x = convert<int16_t>(csvData.at("imu2_mag_x")[index]);
    row.imu2_mag_y = convert<int16_t>(csvData.at("imu2_mag_y")[index]);
    row.imu2_mag_z = convert<int16_t>(csvData.at("imu2_mag_z")[index]);
    row.high_g_accel_x = convert<int16_t>(csvData.at("high_g_accel_x")[index]);
    row.high_g_accel_y = convert<int16_t>(csvData.at("high_g_accel_y")[index]);
    row.high_g_accel_z = convert<int16_t>(csvData.at("high_g_accel_z")[index]);
    row.baro1_temp = convert<double>(csvData.at("baro1_temp")[index]);
    row.baro1_pres = convert<double>(csvData.at("baro1_pres")[index]);
    row.baro2_temp = convert<double>(csvData.at("baro2_temp")[index]);
    row.baro2_pres = convert<double>(csvData.at("baro2_pres")[index]);
    row.gps_lat = convert<double>(csvData.at("gps_lat")[index]);
    row.gps_long = convert<double>(csvData.at("gps_long")[index]);
    row.gps_alt = convert<double>(csvData.at("gps_alt")[index]);
    row.battery_voltage = convert<double>(csvData.at("battery_voltage")[index]);
    row.pyro_cont = convert<uint8_t>(csvData.at("pyro_cont")[index]);  // @TODO: These values were zero on the provided CSV, assuming to be implemented
    row.heading = convert<uint8_t>(csvData.at("heading")[index]);
    row.vtg = convert<uint8_t>(csvData.at("vtg")[index]);
    row.pos_x = convert<double>(csvData.at("pos_x")[index]);
    row.pos_y = convert<double>(csvData.at("pos_y")[index]);
    row.pos_z = convert<double>(csvData.at("pos_z")[index]);
    row.vel_x = convert<double>(csvData.at("vel_x")[index]);
    row.vel_y = convert<double>(csvData.at("vel_y")[index]);
    row.vel_z = convert<double>(csvData.at("vel_z")[index]);
    row.q_x = convert<double>(csvData.at("q_x")[index]);
    row.q_y = convert<double>(csvData.at("q_y")[index]);
    row.q_z = convert<double>(csvData.at("q_z")[index]);
    row.q_w = convert<double>(csvData.at("q_w")[index]);
    row.state = convert<uint8_t>(csvData.at("state")[index]);
    row.baro_pres_avg = convert<double>(csvData.at("baro_pres_avg")[index]);
    row.gps_lat_mod = convert<double>(csvData.at("gps_lat_mod")[index]);
    row.gps_long_mod = convert<double>(csvData.at("gps_long_mod")[index]);
    row.imu1_accel_x_real = convert<double>(csvData.at("imu1_accel_x_real")[index]);
    row.imu1_accel_y_real = convert<double>(csvData.at("imu1_accel_y_real")[index]);
    row.imu1_accel_z_real = convert<double>(csvData.at("imu1_accel_z_real")[index]);
    row.imu2_accel_x_real = convert<double>(csvData.at("imu2_accel_x_real")[index]);
    row.imu2_accel_y_real = convert<double>(csvData.at("imu2_accel_y_real")[index]);
    row.imu2_accel_z_real = convert<double>(csvData.at("imu2_accel_z_real")[index]);
    row.imu1_gyro_x_real = convert<double>(csvData.at("imu1_gyro_x_real")[index]);
    row.imu1_gyro_y_real = convert<double>(csvData.at("imu1_gyro_y_real")[index]);
    row.imu1_gyro_z_real = convert<double>(csvData.at("imu1_gyro_z_real")[index]);
    row.imu2_gyro_x_real = convert<double>(csvData.at("imu2_gyro_x_real")[index]);
    row.imu2_gyro_y_real = convert<double>(csvData.at("imu2_gyro_y_real")[index]);
    row.imu2_gyro_z_real = convert<double>(csvData.at("imu2_gyro_z_real")[index]);
    row.imu1_mag_x_real = convert<double>(csvData.at("imu1_mag_x_real")[index]);
    row.imu1_mag_y_real = convert<double>(csvData.at("imu1_mag_y_real")[index]);
    row.imu1_mag_z_real = convert<double>(csvData.at("imu1_mag_z_real")[index]);
    row.imu2_mag_x_real = convert<double>(csvData.at("imu2_mag_x_real")[index]);
    row.imu2_mag_y_real = convert<double>(csvData.at("imu2_mag_y_real")[index]);
    row.imu2_mag_z_real = convert<double>(csvData.at("imu2_mag_z_real")[index]);
    row.imu_accel_x_avg = convert<double>(csvData.at("imu_accel_x_avg")[index]);
    row.imu_accel_y_avg = convert<double>(csvData.at("imu_accel_y_avg")[index]);
    row.imu_accel_z_avg = convert<double>(csvData.at("imu_accel_z_avg")[index]);
    row.imu_gyro_x_avg = convert<double>(csvData.at("imu_gyro_x_avg")[index]);
    row.imu_gyro_y_avg = convert<double>(csvData.at("imu_gyro_y_avg")[index]);
    row.imu_gyro_z_avg = convert<double>(csvData.at("imu_gyro_z_avg")[index]);
    row.imu_mag_x_avg = convert<double>(csvData.at("imu_mag_x_avg")[index]);
    row.imu_mag_y_avg = convert<double>(csvData.at("imu_mag_y_avg")[index]);
    row.imu_mag_z_avg = convert<double>(csvData.at("imu_mag_z_avg")[index]);
    row.high_g_accel_x_real = convert<double>(csvData.at("high_g_accel_x_real")[index]);
    row.high_g_accel_y_real = convert<double>(csvData.at("high_g_accel_y_real")[index]);
    row.high_g_accel_z_real = convert<double>(csvData.at("high_g_accel_z_real")[index]);
    row.baro_temp_avg = convert<double>(csvData.at("baro_temp_avg")[index]);

    return row;
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
aeroCsvRow_s CustomCsvParser::convertRow(std::string &line) {
    std::vector<std::string> tokens = splitCSV(line);
    aeroCsvRow_s row;
    row.timestamp_s = convert<uint32_t>(tokens[0]);
    row.timestamp_ms = convert<uint32_t>(tokens[1]);
    row.imu1_accel_x = convert<int16_t>(tokens[2]);
    row.imu1_accel_y = convert<int16_t>(tokens[3]);
    row.imu1_accel_z = convert<int16_t>(tokens[4]);
    row.imu1_gyro_x = convert<int16_t>(tokens[5]);
    row.imu1_gyro_y = convert<int16_t>(tokens[6]);
    row.imu1_gyro_z = convert<int16_t>(tokens[7]);
    row.imu1_mag_x = convert<int16_t>(tokens[8]);
    row.imu1_mag_y = convert<int16_t>(tokens[9]);
    row.imu1_mag_z = convert<int16_t>(tokens[10]);
    row.imu2_accel_x = convert<int16_t>(tokens[11]);
    row.imu2_accel_y = convert<int16_t>(tokens[12]);
    row.imu2_accel_z = convert<int16_t>(tokens[13]);
    row.imu2_gyro_x = convert<int16_t>(tokens[14]);
    row.imu2_gyro_y = convert<int16_t>(tokens[15]);
    row.imu2_gyro_z = convert<int16_t>(tokens[16]);
    row.imu2_mag_x = convert<int16_t>(tokens[17]);
    row.imu2_mag_y = convert<int16_t>(tokens[18]);
    row.imu2_mag_z = convert<int16_t>(tokens[19]);
    row.high_g_accel_x = convert<int16_t>(tokens[20]);
    row.high_g_accel_y = convert<int16_t>(tokens[21]);
    row.high_g_accel_z = convert<int16_t>(tokens[22]);
    row.baro1_temp = convert<double>(tokens[23]);
    row.baro1_pres = convert<double>(tokens[24]);
    row.baro2_temp = convert<double>(tokens[25]);
    row.baro2_pres = convert<double>(tokens[26]);
    row.gps_lat = convert<double>(tokens[27]);
    row.gps_long = convert<double>(tokens[28]);
    row.gps_alt = convert<double>(tokens[29]);
    row.battery_voltage = convert<double>(tokens[30]);
    row.pyro_cont = convert<uint8_t>(tokens[31]);
    row.heading = convert<uint8_t>(tokens[32]);
    row.vtg = convert<uint8_t>(tokens[33]);
    row.pos_x = convert<double>(tokens[34]);
    row.pos_y = convert<double>(tokens[35]);
    row.pos_z = convert<double>(tokens[36]);
    row.vel_x = convert<double>(tokens[37]);
    row.vel_y = convert<double>(tokens[38]);
    row.vel_z = convert<double>(tokens[39]);
    row.q_x = convert<double>(tokens[40]);
    row.q_y = convert<double>(tokens[41]);
    row.q_z = convert<double>(tokens[42]);
    row.q_w = convert<double>(tokens[43]);
    row.state = convert<uint8_t>(tokens[44]);
    row.baro_pres_avg = convert<double>(tokens[45]);
    row.gps_lat_mod = convert<double>(tokens[46]);
    row.gps_long_mod = convert<double>(tokens[47]);
    row.imu1_accel_x_real = convert<double>(tokens[48]);
    row.imu1_accel_y_real = convert<double>(tokens[49]);
    row.imu1_accel_z_real = convert<double>(tokens[50]);
    row.imu2_accel_x_real = convert<double>(tokens[51]);
    row.imu2_accel_y_real = convert<double>(tokens[52]);
    row.imu2_accel_z_real = convert<double>(tokens[53]);
    row.imu1_gyro_x_real = convert<double>(tokens[54]);
    row.imu1_gyro_y_real = convert<double>(tokens[55]);
    row.imu1_gyro_z_real = convert<double>(tokens[56]);
    row.imu2_gyro_x_real = convert<double>(tokens[57]);
    row.imu2_gyro_y_real = convert<double>(tokens[58]);
    row.imu2_gyro_z_real = convert<double>(tokens[59]);
    row.imu1_mag_x_real = convert<double>(tokens[60]);
    row.imu1_mag_y_real = convert<double>(tokens[61]);
    row.imu1_mag_z_real = convert<double>(tokens[62]);
    row.imu2_mag_x_real = convert<double>(tokens[63]);
    row.imu2_mag_y_real = convert<double>(tokens[64]);
    row.imu2_mag_z_real = convert<double>(tokens[65]);
    row.imu_accel_x_avg = convert<double>(tokens[66]);
    row.imu_accel_y_avg = convert<double>(tokens[67]);
    row.imu_accel_z_avg = convert<double>(tokens[68]);
    row.imu_gyro_x_avg = convert<double>(tokens[69]);
    row.imu_gyro_y_avg = convert<double>(tokens[70]);
    row.imu_gyro_z_avg = convert<double>(tokens[71]);
    row.imu_mag_x_avg = convert<double>(tokens[72]);
    row.imu_mag_y_avg = convert<double>(tokens[73]);
    row.imu_mag_z_avg = convert<double>(tokens[74]);
    row.high_g_accel_x_real = convert<double>(tokens[75]);
    row.high_g_accel_y_real = convert<double>(tokens[76]);
    row.high_g_accel_z_real = convert<double>(tokens[77]);
    row.baro_temp_avg = convert<double>(tokens[78]);

    return row;
}

int8_t CustomCsvParser::getRow(size_t index, aeroCsvRow_s &row) {
    if (index >= this->m_csv.size()) {
        return -1;
    }

    row = this->m_csv.at(index);

    // return 0 for successful completion
    return 0;
}

size_t CustomCsvParser::getSize() const {
    return this->m_csv.size();
}

