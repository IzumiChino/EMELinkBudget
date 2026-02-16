#define _CRT_SECURE_NO_WARNINGS
#include "AstronomyAPIClient.h"
#include "SimpleHttpClient.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <vector>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AstronomyAPIClient::AstronomyAPIClient() : m_lastError("") {
}

std::string AstronomyAPIClient::formatTime(std::time_t time) {
    std::tm* timeInfo = std::gmtime(&time);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (timeInfo->tm_year + 1900) << "-"
        << std::setw(2) << (timeInfo->tm_mon + 1) << "-"
        << std::setw(2) << timeInfo->tm_mday << "T"
        << std::setw(2) << timeInfo->tm_hour << ":"
        << std::setw(2) << timeInfo->tm_min << ":"
        << std::setw(2) << timeInfo->tm_sec << "Z";
    return oss.str();
}

std::string AstronomyAPIClient::buildAPIUrl(
    std::time_t time,
    double lat,
    double lon) {

    std::ostringstream url;

    url << "https://ssd.jpl.nasa.gov/api/horizons.api?";

    url << "COMMAND='301'";

    url << "&CENTER='coord@399'";
    url << "&COORD_TYPE='GEODETIC'";
    url << "&SITE_COORD='" << std::fixed << std::setprecision(6)
        << lon << "," << lat << ",0'";

    std::string timeStr = formatTime(time);
    url << "&START_TIME='" << timeStr << "'";

    std::string timeStrStop = formatTime(time + 60);
    url << "&STOP_TIME='" << timeStrStop << "'";
    url << "&STEP_SIZE='1m'";

    url << "&QUANTITIES='1,20'";

    url << "&CSV_FORMAT='YES'";
    url << "&CAL_FORMAT='CAL'";
    url << "&TIME_DIGITS='FRACSEC'";
    url << "&ANG_FORMAT='DEG'";
    url << "&RANGE_UNITS='KM'";

    return url.str();
}

bool AstronomyAPIClient::extractJsonValue(
    const std::string& json,
    const std::string& key,
    std::string& value) {

    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);

    if (keyPos == std::string::npos) {
        return false;
    }

    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return false;
    }

    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() &&
           (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }

    if (json[valueStart] == '\"') {
        valueStart++;
        size_t valueEnd = valueStart;
        while (valueEnd < json.length()) {
            if (json[valueEnd] == '\\' && valueEnd + 1 < json.length()) {
                valueEnd += 2;
            } else if (json[valueEnd] == '\"') {
                break;
            } else {
                valueEnd++;
            }
        }
        if (valueEnd >= json.length()) {
            return false;
        }

        std::string rawValue = json.substr(valueStart, valueEnd - valueStart);

        value.clear();
        for (size_t i = 0; i < rawValue.length(); ++i) {
            if (rawValue[i] == '\\' && i + 1 < rawValue.length()) {
                char nextChar = rawValue[i + 1];
                if (nextChar == 'n') {
                    value += '\n';
                    i++;
                } else if (nextChar == 't') {
                    value += '\t';
                    i++;
                } else if (nextChar == 'r') {
                    value += '\r';
                    i++;
                } else if (nextChar == '\\') {
                    value += '\\';
                    i++;
                } else if (nextChar == '\"') {
                    value += '\"';
                    i++;
                } else {
                    value += rawValue[i];
                }
            } else {
                value += rawValue[i];
            }
        }
    } else {
        size_t valueEnd = valueStart;
        while (valueEnd < json.length() &&
               (std::isdigit(json[valueEnd]) ||
                json[valueEnd] == '.' ||
                json[valueEnd] == '-' ||
                json[valueEnd] == 'e' ||
                json[valueEnd] == 'E' ||
                json[valueEnd] == '+')) {
            valueEnd++;
        }
        value = json.substr(valueStart, valueEnd - valueStart);
    }

    return !value.empty();
}

bool AstronomyAPIClient::extractNestedValue(
    const std::string& json,
    const std::string& path,
    double& value) {

    std::string currentJson = json;
    std::istringstream pathStream(path);
    std::string key;

    while (std::getline(pathStream, key, '.')) {
        std::string strValue;
        if (!extractJsonValue(currentJson, key, strValue)) {
            return false;
        }

        if (pathStream.eof()) {
            try {
                value = std::stod(strValue);
                return true;
            } catch (...) {
                return false;
            }
        }

        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = currentJson.find(searchKey);
        if (keyPos == std::string::npos) {
            return false;
        }

        size_t bracePos = currentJson.find('{', keyPos);
        if (bracePos == std::string::npos) {
            return false;
        }

        currentJson = currentJson.substr(bracePos);
    }

    return false;
}

bool AstronomyAPIClient::parseResponse(
    const std::string& response,
    MoonData& result) {

    if (response.empty()) {
        m_lastError = "Empty response from API";
        return false;
    }

    std::string dataText = response;

    if (response[0] == '{') {
        std::string resultValue;
        if (extractJsonValue(response, "result", resultValue)) {
            dataText = resultValue;
        } else {
            m_lastError = "Could not extract 'result' field from JSON response";
            return false;
        }
    }

    size_t soePos = dataText.find("$$SOE");
    size_t eoePos = dataText.find("$$EOE");

    if (soePos == std::string::npos || eoePos == std::string::npos) {
        m_lastError = "Could not find data markers ($$SOE/$$EOE) in response";
        return false;
    }

    std::string dataSection = dataText.substr(soePos + 5, eoePos - soePos - 5);

    std::istringstream dataStream(dataSection);
    std::string dataLine;
    while (std::getline(dataStream, dataLine)) {
        size_t start = dataLine.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) {
            dataLine = dataLine.substr(start);
            if (!dataLine.empty() && dataLine[0] != '#') {
                break;
            }
        }
    }

    if (dataLine.empty()) {
        m_lastError = "No data found in response";
        return false;
    }

    std::istringstream lineStream(dataLine);
    std::string field;
    std::vector<std::string> fields;

    while (std::getline(lineStream, field, ',')) {
        size_t start = field.find_first_not_of(" \t");
        size_t end = field.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            fields.push_back(field.substr(start, end - start + 1));
        } else {
            fields.push_back("");
        }
    }

    if (fields.size() < 6) {
        m_lastError = "Insufficient fields in CSV response";
        return false;
    }

    try {
        result.ra_deg = std::stod(fields[3]);
        result.dec_deg = std::stod(fields[4]);
        result.distance_km = std::stod(fields[5]);

        if (result.ra_deg < 0 || result.ra_deg > 360) {
            m_lastError = "RA out of valid range (0-360)";
            return false;
        }

        if (result.dec_deg < -90 || result.dec_deg > 90) {
            m_lastError = "DEC out of valid range (-90 to 90)";
            return false;
        }

        if (result.distance_km < 300000 || result.distance_km > 500000) {
            m_lastError = "Distance out of reasonable range (300000-500000 km)";
            return false;
        }

        result.azimuth_deg = 0.0;
        result.elevation_deg = 0.0;
        result.source = "JPL Horizons";
        result.valid = true;

        return true;

    } catch (const std::exception& e) {
        m_lastError = std::string("Failed to parse numeric values: ") + e.what();
        return false;
    }
}

//========== Fetch Moon Position ==========

bool AstronomyAPIClient::fetchMoonPosition(
    std::time_t observationTime,
    double observerLat_deg,
    double observerLon_deg,
    MoonData& result) {

    m_lastError.clear();
    result.valid = false;

    // Build API URL
    std::string url = buildAPIUrl(observationTime, observerLat_deg, observerLon_deg);

    std::cout << "[DEBUG] API URL: " << url << std::endl;

    // Fetch data
    std::string response;
    if (!SimpleHttpClient::fetchUrl(url, response)) {
        m_lastError = "Failed to fetch data from API (network error or API unavailable)";
        return false;
    }

    std::cout << "[DEBUG] Response length: " << response.length() << " bytes" << std::endl;
    std::cout << "[DEBUG] First 500 chars of response:\n"
              << response.substr(0, std::min(size_t(500), response.length())) << std::endl;

    // Parse response
    if (!parseResponse(response, result)) {
        // m_lastError already set by parseResponse
        return false;
    }

    return true;
}
