#define _CRT_SECURE_NO_WARNINGS
#include "NOAAGlotecReader.h"
#include "SimpleHttpClient.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <regex>
#include <iostream>

NOAAGlotecReader::NOAAGlotecReader()
    : m_baseUrl("https://services.swpc.noaa.gov/products/glotec/geojson_2d_urt/") {
}

std::tm NOAAGlotecReader::roundToNearest5Minutes(const std::tm& time, bool roundDown) const {
    std::tm rounded = time;

    int minutes = time.tm_min;
    int lastDigit = minutes % 10;

    if (roundDown) {
        int rounded_min = (minutes / 10) * 10 + 5;
        if (lastDigit < 5) {
            rounded_min -= 10;
        }

        if (rounded_min < 0) {
            rounded_min = 55;
            rounded.tm_hour -= 1;
            if (rounded.tm_hour < 0) {
                rounded.tm_hour = 23;
                rounded.tm_mday -= 1;
            }
        }

        rounded.tm_min = rounded_min;
    } else {
        int rounded_min = (minutes / 10) * 10 + 5;
        if (lastDigit > 5) {
            rounded_min += 10;
        }

        if (rounded_min >= 60) {
            rounded_min = 5;
            rounded.tm_hour += 1;
            if (rounded.tm_hour >= 24) {
                rounded.tm_hour = 0;
                rounded.tm_mday += 1;
            }
        }

        rounded.tm_min = rounded_min;
    }

    rounded.tm_sec = 0;
    return rounded;
}

std::string NOAAGlotecReader::getDataUrl(const std::tm& time) const {
    std::tm rounded = roundToNearest5Minutes(time, true);

    std::ostringstream oss;
    oss << m_baseUrl << "glotec_icao_"
        << std::setfill('0')
        << std::setw(4) << (rounded.tm_year + 1900)
        << std::setw(2) << (rounded.tm_mon + 1)
        << std::setw(2) << rounded.tm_mday
        << "T"
        << std::setw(2) << rounded.tm_hour
        << std::setw(2) << rounded.tm_min
        << std::setw(2) << rounded.tm_sec
        << "Z.geojson";

    return oss.str();
}

bool NOAAGlotecReader::parseGeoJson(const std::string& jsonContent, GlotecData& data) {
    data.tecValues.clear();
    data.isValid = false;

    std::regex coordRegex(R"("coordinates"\s*:\s*\[\s*(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*)\s*\])");
    std::regex tecRegex(R"("tec"\s*:\s*(-?\d+\.?\d*))");

    auto coordBegin = std::sregex_iterator(jsonContent.begin(), jsonContent.end(), coordRegex);
    auto coordEnd = std::sregex_iterator();
    auto tecBegin = std::sregex_iterator(jsonContent.begin(), jsonContent.end(), tecRegex);

    std::vector<double> lons, lats;
    std::vector<float> tecs;

    for (auto it = coordBegin; it != coordEnd; ++it) {
        std::smatch match = *it;
        double lon = std::stod(match[1].str());
        double lat = std::stod(match[2].str());
        lons.push_back(lon);
        lats.push_back(lat);
    }

    for (auto it = tecBegin; it != std::sregex_iterator(); ++it) {
        std::smatch match = *it;
        float tec = std::stof(match[1].str());
        tecs.push_back(tec);
    }

    if (lons.size() != tecs.size() || lons.empty()) {
        return false;
    }

    double minLat = *std::min_element(lats.begin(), lats.end());
    double maxLat = *std::max_element(lats.begin(), lats.end());
    double minLon = *std::min_element(lons.begin(), lons.end());

    data.latStart = minLat;
    data.lonStart = minLon;

    std::vector<double> uniqueLats = lats;
    std::sort(uniqueLats.begin(), uniqueLats.end());
    uniqueLats.erase(std::unique(uniqueLats.begin(), uniqueLats.end()), uniqueLats.end());

    if (uniqueLats.size() > 1) {
        data.latStep = uniqueLats[1] - uniqueLats[0];
        data.numLat = static_cast<int>(uniqueLats.size());
    }

    data.numLon = 72;
    data.lonStep = 5.0;

    data.tecValues.resize(data.numLon * data.numLat, 0.0f);

    for (size_t i = 0; i < lons.size(); ++i) {
        int col = static_cast<int>(std::round((lons[i] - data.lonStart) / data.lonStep));
        int row = static_cast<int>(std::round((lats[i] - data.latStart) / data.latStep));

        if (col >= 0 && col < data.numLon && row >= 0 && row < data.numLat) {
            int idx = getGridIndex(col, row, data.numLon);
            data.tecValues[idx] = tecs[i];
        }
    }

    data.isValid = true;
    return true;
}

int NOAAGlotecReader::getGridIndex(int col, int row, int numCols) const {
    return col + row * numCols;
}

double NOAAGlotecReader::bilinearInterpolate(const GlotecData& data, double lat, double lon) const {
    if (!data.isValid || data.tecValues.empty()) {
        return 0.0;
    }

    while (lon < -180.0) lon += 360.0;
    while (lon > 180.0) lon -= 360.0;

    double colFloat = (lon - data.lonStart) / data.lonStep;
    double rowFloat = (lat - data.latStart) / data.latStep;

    int col0 = static_cast<int>(std::floor(colFloat));
    int row0 = static_cast<int>(std::floor(rowFloat));
    int col1 = col0 + 1;
    int row1 = row0 + 1;

    if (col0 < 0 || col1 >= data.numLon || row0 < 0 || row1 >= data.numLat) {
        if (col0 >= 0 && col0 < data.numLon && row0 >= 0 && row0 < data.numLat) {
            return data.tecValues[getGridIndex(col0, row0, data.numLon)];
        }
        return 0.0;
    }

    double dx = colFloat - col0;
    double dy = rowFloat - row0;

    float q00 = data.tecValues[getGridIndex(col0, row0, data.numLon)];
    float q10 = data.tecValues[getGridIndex(col1, row0, data.numLon)];
    float q01 = data.tecValues[getGridIndex(col0, row1, data.numLon)];
    float q11 = data.tecValues[getGridIndex(col1, row1, data.numLon)];

    double tec = q00 * (1 - dx) * (1 - dy) +
                 q10 * dx * (1 - dy) +
                 q01 * (1 - dx) * dy +
                 q11 * dx * dy;

    return tec;
}

bool NOAAGlotecReader::getTecAtLocation(const GlotecData& data, double lat, double lon, double& tec) {
    if (!data.isValid) {
        return false;
    }

    tec = bilinearInterpolate(data, lat, lon);
    return true;
}

bool NOAAGlotecReader::fetchTecData(const std::tm& requestTime, GlotecData& data) {
    std::string url = getDataUrl(requestTime);
    std::string jsonContent;
    int statusCode = 0;
    std::string errorMsg;

    if (!SimpleHttpClient::fetchUrlWithStatus(url, jsonContent, statusCode, errorMsg)) {
        std::cout << "[DEBUG] First attempt failed: " << errorMsg << std::endl;

        std::tm roundedTime = roundToNearest5Minutes(requestTime, false);
        url = getDataUrl(roundedTime);
        std::cout << "[DEBUG] Trying alternate URL: " << url << std::endl;

        if (!SimpleHttpClient::fetchUrlWithStatus(url, jsonContent, statusCode, errorMsg)) {
            std::cout << "[DEBUG] Second attempt failed: " << errorMsg << std::endl;

            if (statusCode == 404) {
                std::cout << "[DEBUG] Data not available yet (404), trying recent historical data..." << std::endl;

                for (int minutesBack = 10; minutesBack <= 30; minutesBack += 10) {
                    std::tm historicalTime = requestTime;
                    historicalTime.tm_min -= minutesBack;

                    if (historicalTime.tm_min < 0) {
                        historicalTime.tm_min += 60;
                        historicalTime.tm_hour -= 1;
                        if (historicalTime.tm_hour < 0) {
                            historicalTime.tm_hour += 24;
                            historicalTime.tm_mday -= 1;
                        }
                    }

                    std::tm roundedHistorical = roundToNearest5Minutes(historicalTime, true);
                    url = getDataUrl(roundedHistorical);

                    std::cout << "[DEBUG] Trying " << minutesBack << " minute(s) back: " << url << std::endl;

                    if (SimpleHttpClient::fetchUrlWithStatus(url, jsonContent, statusCode, errorMsg)) {
                        std::cout << "[DEBUG] Found data from " << minutesBack << " minute(s) ago" << std::endl;
                        data.timestamp = roundedHistorical;
                        bool parseSuccess = parseGeoJson(jsonContent, data);
                        if (parseSuccess) {
                            std::cout << "[!] Using data from " << minutesBack << " minute(s) ago (NOAA data has ~10-30 min delay)" << std::endl;
                        }
                        return parseSuccess;
                    }
                }

                std::cout << "[DEBUG] No data found in past 30 minutes" << std::endl;
            }

            return false;
        }
    }

    std::cout << "[DEBUG] HTTP 200 OK, response size: " << jsonContent.length() << " bytes" << std::endl;

    data.timestamp = roundToNearest5Minutes(requestTime, true);
    bool parseSuccess = parseGeoJson(jsonContent, data);

    if (!parseSuccess) {
        std::cout << "[DEBUG] GeoJSON parsing failed" << std::endl;
        std::cout << "[DEBUG] First 200 chars: " << jsonContent.substr(0, 200) << std::endl;
    }

    return parseSuccess;
}
