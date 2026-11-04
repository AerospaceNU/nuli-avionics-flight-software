#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SIMULATIONLOGFILE_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SIMULATIONLOGFILE_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

/**
 * @class SimulationLogFile
 * @brief Turns a SillyGoose/SeriousGoose log into one sim sample per 10 ms tick, the same way SillyGooseTool's simulation does
 * @details Reads the first 9 columns every board logs (timestampMs, pressurePa, barometerTemperatureK, accel x/y/z, gyro x/y/z),
 * keeps PREFLIGHT_KEEP_MS before the first densely logged row, and interpolates the sparse ground logging up to the tick rate.
 */
class SimulationLogFile {
public:
    using Sample = std::array<float, 8>;

    bool load(const char* path) {
        std::ifstream file(path);
        if (!file) return false;
        std::vector<std::array<double, 9>> rows;
        for (std::string line; std::getline(file, line);) {
            std::array<double, 9> row{};
            if (parseRow(line, row)) rows.push_back(row);
        }
        std::stable_sort(rows.begin(), rows.end(), [](const std::array<double, 9>& a, const std::array<double, 9>& b) { return a[0] < b[0]; });
        trimPreflight(rows);
        if (rows.empty() || rows.back()[0] - rows.front()[0] > MAX_SPAN_MS) return false;
        buildSamples(rows);
        return !m_samples.empty();
    }

    const std::vector<Sample>& getSamples() const {
        return m_samples;
    }

private:
    static constexpr double GAP_THRESHOLD_MS = 30.0;
    static constexpr double PREFLIGHT_KEEP_MS = 35000.0;
    static constexpr double TICK_MS = 10.0;
    static constexpr double MAX_SPAN_MS = 12 * 3600 * 1000.0;

    static bool parseRow(const std::string& line, std::array<double, 9>& row) {
        const size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos || !isdigit((unsigned char)line[start])) return false;
        const char* cursor = line.c_str() + start;
        for (double& value: row) {
            char* end;
            value = strtod(cursor, &end);
            if (end == cursor || std::isnan(value) || (*end != '\t' && *end != '\0' && *end != '\r')) return false;
            cursor = *end == '\t' ? end + 1 : end;
        }
        return true;
    }

    static void trimPreflight(std::vector<std::array<double, 9>>& rows) {
        for (size_t i = 1; i < rows.size(); i++) {
            if (rows[i][0] - rows[i - 1][0] <= GAP_THRESHOLD_MS) {
                const double cutoffMs = rows[i][0] - PREFLIGHT_KEEP_MS;
                rows.erase(rows.begin(), std::find_if(rows.begin(), rows.end(), [cutoffMs](const std::array<double, 9>& r) { return r[0] >= cutoffMs; }));
                return;
            }
        }
    }

    void buildSamples(const std::vector<std::array<double, 9>>& rows) {
        m_samples.clear();
        for (size_t i = 0; i < rows.size(); i++) {
            if (i > 0 && rows[i][0] - rows[i - 1][0] > GAP_THRESHOLD_MS) {
                const std::array<double, 9>& previous = rows[i - 1];
                const double dtMs = rows[i][0] - previous[0];
                for (double t = previous[0] + TICK_MS; t < rows[i][0]; t += TICK_MS) {
                    const double ratio = (t - previous[0]) / dtMs;
                    Sample sample;
                    for (size_t j = 0; j < sample.size(); j++) sample[j] = (float)(previous[j + 1] + (rows[i][j + 1] - previous[j + 1]) * ratio);
                    m_samples.push_back(sample);
                }
            }
            Sample sample;
            for (size_t j = 0; j < sample.size(); j++) sample[j] = (float)rows[i][j + 1];
            m_samples.push_back(sample);
        }
    }

    std::vector<Sample> m_samples;
};

#endif // PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_SIMULATIONLOGFILE_H
