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

/**
 * @class   CustomCsvParser
 * @brief   Parses a read-only CSV into a usable vector object.
 * @details Provides functionality on retrieving data from the CSV. Enforces
 *          expectations of the CSV structure. The order of the input data must
 *          reflect the order in FlightDataRow_s. This class presupposes the
 *          structure of the input CSV.
 */
class CustomCsvParser {
public:
    /**
     * @struct  FlightDataRow_s
     * @brief   A single row of the input CSV.
     */
    typedef struct {
        uint32_t timestamp_s;
        uint32_t timestamp_ms;


        struct {
            double x;
            double y;
            double z;
        } imu1_accel_s;

        struct {
            double x;
            double y;
            double z;
        } imu1_gyro_s;

        struct {
            double x;
            double y;
            double z;
        } imu1_mag_s;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } imu2_accel_s;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } imu2_gyro_s;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } imu2_mag_s;

        struct {
            int16_t x;
            int16_t y;
            int16_t z;
        } high_g_accel_s;

        struct {
            double temp;
            double pres;
        } baro1_s;

        struct {
            double temp;
            double pres;
        } baro2_s;

        struct {
            double latitude;
            double longitude;
            double alt;

        } gps_s;

        double battery_voltage;
        uint8_t pyro_cont;  // @TODO: These values were zero on the provided CSV, assuming to be implemented
        uint8_t heading;
        uint8_t vtg;

        struct {
            double x;
            double y;
            double z;
        } pos_s;

        struct {
            double x;
            double y;
            double z;
        } vel_s;

        struct {
            double x;
            double y;
            double z;
            double w;
        } q_s;

        uint8_t state;
        double baro_pres_avg;

        struct {
            double latitude;
            double longitude;
        } gps_mod_s;

        struct {
            double x;
            double y;
            double z;
        } imu1_accel_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu2_accel_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu1_gyro_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu2_gyro_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu1_mag_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu2_mag_real_s;

        struct {
            double x;
            double y;
            double z;
        } imu_accel_avg_s;

        struct {
            double x;
            double y;
            double z;
        } imu_gyro_avg_s;

        struct {
            double x;
            double y;
            double z;
        } imu_mag_avg_s;

        struct {
            double x;
            double y;
            double z;
        } high_g_accel_real_s;


        double baro_temp_avg;
    } FlightDataRow_s;

    /**
     * @brief Default constructor.
     */
    CustomCsvParser() = default;

    /**
     * @brief Default destructor
     */
    ~CustomCsvParser() = default;


    /**
     * @brief   Reads CSV into a vector of FlightDataRow_s.
     * @param filename  The name of the file.
     * @param header    Defaults to false. True if CSV has header.
     * @return  0 for success, 1 for failure.
     */
    int8_t parse(const std::string &filename, bool header);

    /**
     * @brief   Getter. Retrieves a row of the AeroCsvRow vector.
     * @param index The row which to retrieve data from.
     * @return  A FlightDataRow_s object.
     */
    [[nodiscard]] FlightDataRow_s getRow(size_t index) const;

    /**
     * @brief   Getter. Gets the size of the FlightDataRow_s vector.
     * @return  A size_t representing the size of the vector of FlightDataRow_s.
     */
    [[nodiscard]] size_t getSize() const;

private:
    /**
     * @brief   Converts a string to a FlightDataRow_s
     * @param line  Input line of CSV. MUST conform to FlightDataRow_s structure.
     * @return  A FlightRowData_s object.
     */
    FlightDataRow_s convertRow(std::string &line);

    /**
     * @brief   A utility function to convert a string value to type T.
     * @tparam T    The type to convert a string to.
     * @param value String to attempt to convert.
     * @return  String value in type T.
     */
    template<typename T>
    T convertToTypeT(const std::string &value);

    /**
     * @brief   Utility function to split a line of a csv into a vector of tokens.
     * @param line  Line in a CSV.
     * @return  A vector of string tokens.
     */
    static std::vector<std::string> splitCSV(const std::string &line);

    std::vector<FlightDataRow_s> m_csv;     ///< Represents the CSV, each row is a FlightDataRow_s
};


#endif //DESKTOP_CUSTOMCSVPARSER_H
