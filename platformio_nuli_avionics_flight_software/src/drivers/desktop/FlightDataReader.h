#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include <limits>

class FlightDataReader {
public:
    FlightDataReader() = default;

    // Load a delimited text file. Defaults to tab-separated.
    // If startTimestamp >= 0, replay anchors at that timestamp instead of the file's first row,
    // so the sim begins at a specific point in the recording.
    void setup(const std::string &fileName, char separator = '\t', int64_t startTimestamp = -1) {
        separator_ = separator;
        file_.open(fileName);
        if (!file_.is_open()) throw std::runtime_error("Cannot open data file");

        std::string headerLine;
        if (!std::getline(file_, headerLine)) throw std::runtime_error("Empty data file");

        parseHeader(headerLine);

        // Load all remaining lines into memory
        lines_.clear();
        std::string line;
        while (std::getline(file_, line)) {
            if (!line.empty()) lines_.push_back(parseLine(line));
        }
        currentIndex_ = 0;
        firstTimestamp_ = startTimestamp; // negative → anchor on first row at first interpolateNext call
    }

    // Read next row without interpolation
    bool nextLine() {
        if (currentIndex_ >= lines_.size()) {
            std::exit(0); // exit program when data ends
        }
        currentRow_ = lines_[currentIndex_++];
        return true;
    }

    // Interpolate row for relative time (t=0 at first call, or from startTimestamp if set)
    bool interpolateNext(int64_t relativeTimeMs) {
        if (currentIndex_ >= lines_.size()) {
            std::exit(0); // exit program when data ends
        }
        if (lines_.empty()) {
            return false;
        }

        if (firstTimestamp_ < 0) {
            firstTimestamp_ = convert<int64_t>(lines_[0].at(timeKey_));
        }

        int64_t csvTime = firstTimestamp_ + relativeTimeMs;

        int64_t firstTime = convert<int64_t>(lines_[0].at(timeKey_));
        if (csvTime <= firstTime) {
            currentRow_ = lines_[0];
            currentIndex_ = 1;
            return true;
        }

        while (currentIndex_ < lines_.size() &&
               convert<int64_t>(lines_[currentIndex_].at(timeKey_)) < csvTime) {
            ++currentIndex_;
        }

        if (currentIndex_ >= lines_.size()) {
            currentRow_ = lines_.back();
            return true;
        }

        const auto &prev = lines_[currentIndex_ - 1];
        const auto &next = lines_[currentIndex_];

        int64_t t0 = convert<int64_t>(prev.at(timeKey_));
        int64_t t1 = convert<int64_t>(next.at(timeKey_));
        double alpha = double(csvTime - t0) / double(t1 - t0);

        currentRow_.clear();
        for (const auto &key : headers_) {
            if (key == timeKey_) {
                currentRow_[key] = std::to_string(csvTime);
            } else {
                double v0, v1;
                if (tryGetDouble(prev, key, v0) && tryGetDouble(next, key, v1)) {
                    currentRow_[key] = std::to_string(v0 + alpha * (v1 - v0));
                } else {
                    auto it = prev.find(key);
                    currentRow_[key] = (it != prev.end() && !it->second.empty()) ? it->second : "nan";
                }
            }
        }

        return true;
    }

    // Type-safe access to current row
    template<typename T>
    T getKey(const std::string &key) const {
        auto it = currentRow_.find(key);
        if (it == currentRow_.end() || it->second.empty()) {
            if constexpr (std::is_floating_point<T>::value)
                return std::numeric_limits<T>::quiet_NaN();
            else
                return T{};
        }
        return convert<T>(it->second);
    }

private:
    std::ifstream file_;
    std::vector<std::unordered_map<std::string, std::string>> lines_;
    std::unordered_map<std::string, std::string> currentRow_;
    std::vector<std::string> headers_;
    size_t currentIndex_ = 0;
    int64_t firstTimestamp_ = -1;
    char separator_ = '\t';
    const std::string timeKey_ = "timestampMs";


    void parseHeader(const std::string &line) {
        std::string trimmed = line;
        trimTrailingSeparator(trimmed);
        headers_ = splitLine(trimmed);
    }

    std::unordered_map<std::string, std::string> parseLine(const std::string &line) {
        std::string trimmed = line;
        trimTrailingSeparator(trimmed);

        std::unordered_map<std::string, std::string> row;
        auto tokens = splitLine(trimmed);
        for (size_t i = 0; i < headers_.size(); ++i) {
            if (i < tokens.size()) row[headers_[i]] = stripQuotes(tokens[i]);
            else row[headers_[i]] = ""; // missing values
        }
        return row;
    }

    void trimTrailingSeparator(std::string &s) const {
        while (!s.empty() && s.back() == separator_) s.pop_back();
    }

    // Split line on the configured separator, respecting quotes
    std::vector<std::string> splitLine(const std::string &line) const {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes; // toggle quote state
            } else if (c == separator_ && !inQuotes) {
                result.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        result.push_back(current);
        return result;
    }

    // Remove surrounding quotes if present
    static std::string stripQuotes(const std::string &s) {
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }

    template<typename T>
    static T convert(const std::string &s) {
        if constexpr (std::is_integral<T>::value) {
            if (s.empty()) return T{};
            return static_cast<T>(std::stoll(s));
        } else if constexpr (std::is_floating_point<T>::value) {
            if (s.empty() || s == "nan") return std::numeric_limits<T>::quiet_NaN();
            return static_cast<T>(std::stod(s));
        } else if constexpr (std::is_same<T, std::string>::value) {
            return s;
        } else {
            static_assert("Unsupported type for FlightDataReader::getKey");
        }
    }


    static bool tryGetDouble(const std::unordered_map<std::string, std::string> &row,
                             const std::string &key, double &out) {
        auto it = row.find(key);
        if (it == row.end() || it->second.empty() || it->second == "nan") return false;

        try {
            out = std::stod(it->second);
            return true;
        } catch (...) {
            return false;
        }
    }

};
