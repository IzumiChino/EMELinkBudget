#include "IonexReader.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

// ========== Constructors ==========

IonexReader::IonexReader()
    : m_filename(""), m_isOpen(false), m_header() {
}

IonexReader::IonexReader(const std::string& filename)
    : m_filename(filename), m_isOpen(false), m_header() {
    open(filename);
}

// ========== File Opening ==========

bool IonexReader::open(const std::string& filename) {
    m_filename = filename;
    m_isOpen = false;
    m_mapPositions.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    if (!parseHeader(file)) {
        return false;
    }

    if (!buildMapIndex(file)) {
        return false;
    }

    m_isOpen = true;
    return true;
}

// ========== Header Parsing ==========

bool IonexReader::parseHeader(std::ifstream& file) {
    std::string line;
    bool foundEnd = false;

    while (std::getline(file, line)) {
        if (line.length() < 60) continue;

        std::string label = line.substr(60);

        if (label.find("IONEX VERSION / TYPE") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.version;
        }
        else if (label.find("EPOCH OF FIRST MAP") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.epochFirst.tm_year >> m_header.epochFirst.tm_mon
                >> m_header.epochFirst.tm_mday >> m_header.epochFirst.tm_hour
                >> m_header.epochFirst.tm_min >> m_header.epochFirst.tm_sec;
            m_header.epochFirst.tm_year -= 1900;
            m_header.epochFirst.tm_mon -= 1;
        }
        else if (label.find("EPOCH OF LAST MAP") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.epochLast.tm_year >> m_header.epochLast.tm_mon
                >> m_header.epochLast.tm_mday >> m_header.epochLast.tm_hour
                >> m_header.epochLast.tm_min >> m_header.epochLast.tm_sec;
            m_header.epochLast.tm_year -= 1900;
            m_header.epochLast.tm_mon -= 1;
        }
        else if (label.find("INTERVAL") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.interval;
        }
        else if (label.find("# OF MAPS IN FILE") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.numMaps;
        }
        else if (label.find("BASE RADIUS") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.baseRadius;
        }
        else if (label.find("HGT1 / HGT2 / DHGT") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.hgt1 >> m_header.hgt2 >> m_header.dhgt;
        }
        else if (label.find("LAT1 / LAT2 / DLAT") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.lat1 >> m_header.lat2 >> m_header.dlat;
            m_header.numLat = static_cast<int>((m_header.lat1 - m_header.lat2) / (-m_header.dlat)) + 1;
        }
        else if (label.find("LON1 / LON2 / DLON") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.lon1 >> m_header.lon2 >> m_header.dlon;
            m_header.numLon = static_cast<int>((m_header.lon2 - m_header.lon1) / m_header.dlon) + 1;
        }
        else if (label.find("EXPONENT") != std::string::npos) {
            std::istringstream iss(line.substr(0, 60));
            iss >> m_header.exponent;
        }
        else if (label.find("END OF HEADER") != std::string::npos) {
            foundEnd = true;
            break;
        }
    }

    return foundEnd;
}

// ========== Map Index Building ==========

bool IonexReader::buildMapIndex(std::ifstream& file) {
    std::string line;
    std::streamoff currentPos = file.tellg();

    while (std::getline(file, line)) {
        if (line.find("START OF TEC MAP") != std::string::npos) {
            std::streamoff mapStartPos = currentPos;

            if (std::getline(file, line) && line.find("EPOCH OF CURRENT MAP") != std::string::npos) {
                std::tm epoch = {};
                std::istringstream iss(line.substr(0, 60));
                iss >> epoch.tm_year >> epoch.tm_mon >> epoch.tm_mday
                    >> epoch.tm_hour >> epoch.tm_min >> epoch.tm_sec;
                epoch.tm_year -= 1900;
                epoch.tm_mon -= 1;
                epoch.tm_isdst = -1;

                std::time_t t = std::mktime(&epoch);
                m_mapPositions[t] = static_cast<long>(mapStartPos);
            }
        }
        currentPos = file.tellg();
    }

    return !m_mapPositions.empty();
}

// ========== TEC Map Loading ==========

bool IonexReader::loadTecMap(std::ifstream& file, long position, TecMap& tecMap) {
    file.clear();
    file.seekg(position);

    std::string line;
    if (!std::getline(file, line) || line.find("START OF TEC MAP") == std::string::npos) {
        return false;
    }

    if (!std::getline(file, line) || line.find("EPOCH OF CURRENT MAP") == std::string::npos) {
        return false;
    }

    std::istringstream iss(line.substr(0, 60));
    iss >> tecMap.epoch.tm_year >> tecMap.epoch.tm_mon >> tecMap.epoch.tm_mday
        >> tecMap.epoch.tm_hour >> tecMap.epoch.tm_min >> tecMap.epoch.tm_sec;
    tecMap.epoch.tm_year -= 1900;
    tecMap.epoch.tm_mon -= 1;

    tecMap.data.resize(m_header.numLat);
    for (int i = 0; i < m_header.numLat; ++i) {
        tecMap.data[i].resize(m_header.numLon, 9999.0);
    }

    int currentLatIdx = 0;

    while (std::getline(file, line)) {
        if (line.find("END OF TEC MAP") != std::string::npos) {
            break;
        }

        if (line.find("LAT/LON1/LON2/DLON/H") != std::string::npos) {
            double lat;
            std::istringstream latIss(line.substr(0, 60));
            latIss >> lat;
            currentLatIdx = latToIndex(lat);

            if (currentLatIdx < 0 || currentLatIdx >= m_header.numLat) {
                continue;
            }

            int lonIdx = 0;
            while (lonIdx < m_header.numLon && std::getline(file, line)) {
                if (line.find("LAT/LON1/LON2/DLON/H") != std::string::npos ||
                    line.find("END OF TEC MAP") != std::string::npos) {
                    file.seekg(-static_cast<long>(line.length()) - 1, std::ios::cur);
                    break;
                }

                for (size_t pos = 0; pos < line.length() && lonIdx < m_header.numLon; pos += 5) {
                    if (pos + 5 <= line.length()) {
                        std::string valueStr = line.substr(pos, 5);
                        int value;
                        std::istringstream valueIss(valueStr);
                        if (valueIss >> value) {
                            if (value != 9999) {
                                tecMap.data[currentLatIdx][lonIdx] = value * std::pow(10.0, m_header.exponent);
                            }
                            lonIdx++;
                        }
                    }
                }
            }
        }
    }

    return true;
}

// ========== TEC Value Retrieval ==========

bool IonexReader::getTecValue(const std::tm& time, double lat, double lon, double& vtec) {
    if (!m_isOpen) {
        return false;
    }

    std::time_t targetTime = tmToTime(time);

    auto it = m_mapPositions.find(targetTime);
    if (it == m_mapPositions.end()) {
        return false;
    }

    std::ifstream file(m_filename);
    if (!file.is_open()) {
        return false;
    }

    TecMap tecMap;
    if (!loadTecMap(file, it->second, tecMap)) {
        return false;
    }

    int latIdx = latToIndex(lat);
    int lonIdx = lonToIndex(lon);

    if (latIdx < 0 || latIdx >= m_header.numLat ||
        lonIdx < 0 || lonIdx >= m_header.numLon) {
        return false;
    }

    vtec = tecMap.data[latIdx][lonIdx];
    return vtec != 9999.0;
}

// ========== Interpolated TEC Value ==========

bool IonexReader::getTecValueInterpolated(const std::tm& time, double lat, double lon, double& vtec) {
    if (!m_isOpen) {
        return false;
    }

    std::time_t targetTime = tmToTime(time);
    std::time_t t1, t2;

    if (!findClosestMaps(time, t1, t2)) {
        return false;
    }

    std::ifstream file(m_filename);
    if (!file.is_open()) {
        return false;
    }

    TecMap map1, map2;
    if (!loadTecMap(file, m_mapPositions[t1], map1)) {
        return false;
    }

    double vtec1 = bilinearInterpolate(map1.data, lat, lon);
    if (vtec1 == 9999.0) {
        return false;
    }

    if (t1 == t2) {
        vtec = vtec1;
        return true;
    }

    if (!loadTecMap(file, m_mapPositions[t2], map2)) {
        return false;
    }

    double vtec2 = bilinearInterpolate(map2.data, lat, lon);
    if (vtec2 == 9999.0) {
        return false;
    }

    double ratio = static_cast<double>(targetTime - t1) / static_cast<double>(t2 - t1);
    vtec = vtec1 + ratio * (vtec2 - vtec1);

    return true;
}

// ========== Helper Functions ==========

std::time_t IonexReader::tmToTime(const std::tm& tm) const {
    std::tm temp = tm;
    temp.tm_isdst = -1;
    return std::mktime(&temp);
}

bool IonexReader::findClosestMaps(const std::tm& time, std::time_t& t1, std::time_t& t2) {
    std::time_t targetTime = tmToTime(time);

    auto it = m_mapPositions.lower_bound(targetTime);

    if (it == m_mapPositions.end()) {
        if (m_mapPositions.empty()) return false;
        auto last = m_mapPositions.rbegin();
        t1 = t2 = last->first;
        return true;
    }

    if (it->first == targetTime) {
        t1 = t2 = it->first;
        return true;
    }

    if (it == m_mapPositions.begin()) {
        t1 = t2 = it->first;
        return true;
    }

    t2 = it->first;
    --it;
    t1 = it->first;
    return true;
}

double IonexReader::bilinearInterpolate(const std::vector<std::vector<double>>& data,
                                        double lat, double lon) {
    double latNorm = (lat - m_header.lat1) / m_header.dlat;
    double lonNorm = (lon - m_header.lon1) / m_header.dlon;

    int lat1Idx = static_cast<int>(std::floor(latNorm));
    int lat2Idx = lat1Idx + 1;
    int lon1Idx = static_cast<int>(std::floor(lonNorm));
    int lon2Idx = lon1Idx + 1;

    lat1Idx = std::max(0, std::min(lat1Idx, m_header.numLat - 1));
    lat2Idx = std::max(0, std::min(lat2Idx, m_header.numLat - 1));
    lon1Idx = std::max(0, std::min(lon1Idx, m_header.numLon - 1));
    lon2Idx = std::max(0, std::min(lon2Idx, m_header.numLon - 1));

    double v11 = data[lat1Idx][lon1Idx];
    double v12 = data[lat1Idx][lon2Idx];
    double v21 = data[lat2Idx][lon1Idx];
    double v22 = data[lat2Idx][lon2Idx];

    if (v11 == 9999.0 || v12 == 9999.0 || v21 == 9999.0 || v22 == 9999.0) {
        return 9999.0;
    }

    double latFrac = latNorm - lat1Idx;
    double lonFrac = lonNorm - lon1Idx;

    double v1 = v11 * (1.0 - lonFrac) + v12 * lonFrac;
    double v2 = v21 * (1.0 - lonFrac) + v22 * lonFrac;

    return v1 * (1.0 - latFrac) + v2 * latFrac;
}

int IonexReader::latToIndex(double lat) const {
    return static_cast<int>(std::round((m_header.lat1 - lat) / (-m_header.dlat)));
}

int IonexReader::lonToIndex(double lon) const {
    return static_cast<int>(std::round((lon - m_header.lon1) / m_header.dlon));
}

double IonexReader::indexToLat(int idx) const {
    return m_header.lat1 + idx * m_header.dlat;
}

double IonexReader::indexToLon(int idx) const {
    return m_header.lon1 + idx * m_header.dlon;
}
