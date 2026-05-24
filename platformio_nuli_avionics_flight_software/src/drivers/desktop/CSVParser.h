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

// #define CSV_CHAR ','
#define CSV_CHAR '\t'

class CSVReader {
public:
    CSVReader() = default;

    // Load CSV file
    void setup(const std::string &fileName) {
        file_.open(fileName);
        if (!file_.is_open()) throw std::runtime_error("Cannot open CSV file");

        std::string headerLine;
        if (!std::getline(file_, headerLine)) throw std::runtime_error("Empty CSV file");

        parseHeader(headerLine);

        // Load all remaining lines into memory
        lines_.clear();
        std::string line;
        while (std::getline(file_, line)) {
            if (!line.empty()) lines_.push_back(parseLine(line));
        }
        currentIndex_ = 0;
        firstCsvTime_ = -1; // reset relative time tracking
    }

    // Read next row without interpolation
    bool nextLine() {
        if (currentIndex_ >= lines_.size()) {
            std::exit(0); // exit program when CSV ends
        }
        currentRow_ = lines_[currentIndex_++];
        return true;
    }

    // Interpolate row for relative time (t=0 at first call)
    bool interpolateNext(int64_t relativeTimeMs) {
        if (currentIndex_ >= lines_.size()) {
            std::exit(0); // exit program when CSV ends
        }
        if (lines_.empty()) {
            return false;
        }

        if (firstCsvTime_ < 0) {
            firstCsvTime_ = convert<int64_t>(lines_[0].at(timeKey_));
        }

        int64_t csvTime = firstCsvTime_ + relativeTimeMs;

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
    int64_t firstCsvTime_ = -1;
    // const std::string timeKey_ = "timestamp_ms";
    const std::string timeKey_ = "timestampMs";


    void parseHeader(const std::string &line) {
        std::string trimmed = line;
        trimTrailingComma(trimmed);
        headers_ = splitCsvLine(trimmed);
    }

    std::unordered_map<std::string, std::string> parseLine(const std::string &line) {
        std::string trimmed = line;
        trimTrailingComma(trimmed);

        std::unordered_map<std::string, std::string> row;
        auto tokens = splitCsvLine(trimmed);
        for (size_t i = 0; i < headers_.size(); ++i) {
            if (i < tokens.size()) row[headers_[i]] = stripQuotes(tokens[i]);
            else row[headers_[i]] = ""; // missing values
        }
        return row;
    }

    static void trimTrailingComma(std::string &s) {
        while (!s.empty() && s.back() == CSV_CHAR) s.pop_back();
    }

    // Split CSV line respecting quotes
    static std::vector<std::string> splitCsvLine(const std::string &line) {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes; // toggle quote state
            } else if (c == CSV_CHAR && !inQuotes) {
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
        std::string clean = s;
        clean.erase(std::remove(clean.begin(), clean.end(), CSV_CHAR), clean.end()); // remove commas

        if constexpr (std::is_integral<T>::value) {
            if (clean.empty()) return T{};
            return static_cast<T>(std::stoll(clean));
        } else if constexpr (std::is_floating_point<T>::value) {
            if (clean.empty() || clean == "nan") return std::numeric_limits<T>::quiet_NaN();
            return static_cast<T>(std::stod(clean));
        } else if constexpr (std::is_same<T, std::string>::value) {
            return clean;
        } else {
            static_assert("Unsupported type for CSVReader::getKey");
        }
    }


    static bool tryGetDouble(const std::unordered_map<std::string, std::string> &row,
                             const std::string &key, double &out) {
        auto it = row.find(key);
        if (it == row.end() || it->second.empty() || it->second == "nan") return false;

        try {
            std::string clean = it->second;
            clean.erase(std::remove(clean.begin(), clean.end(), CSV_CHAR), clean.end()); // remove commas
            out = std::stod(clean);
            return true;
        } catch (...) {
            return false;
        }
    }

};

