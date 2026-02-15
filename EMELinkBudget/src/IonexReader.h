#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <ctime>

// ========== IONEX Data Structures ==========

struct IonexHeader {
    double version = 0.0;
    std::string type;
    std::string description;

    std::tm epochFirst = {};
    std::tm epochLast = {};
    int interval = 0;
    int numMaps = 0;

    double baseRadius = 0.0;
    double hgt1 = 0.0, hgt2 = 0.0, dhgt = 0.0;
    double lat1 = 0.0, lat2 = 0.0, dlat = 0.0;
    double lon1 = 0.0, lon2 = 0.0, dlon = 0.0;
    int exponent = 0;

    int numLat = 0;
    int numLon = 0;
};

struct TecMap {
    std::tm epoch = {};
    std::vector<std::vector<double>> data;
};

// ========== IONEX Reader Class ==========

class IonexReader {
public:
    IonexReader();
    explicit IonexReader(const std::string& filename);

    bool open(const std::string& filename);
    bool isOpen() const { return m_isOpen; }

    const IonexHeader& getHeader() const { return m_header; }

    bool getTecValue(const std::tm& time, double lat, double lon, double& vtec);

    bool getTecValueInterpolated(const std::tm& time, double lat, double lon, double& vtec);

private:
    std::string m_filename;
    bool m_isOpen;
    IonexHeader m_header;

    std::map<std::time_t, long> m_mapPositions;

    bool parseHeader(std::ifstream& file);
    bool buildMapIndex(std::ifstream& file);
    bool loadTecMap(std::ifstream& file, long position, TecMap& tecMap);

    std::time_t tmToTime(const std::tm& tm) const;
    bool findClosestMaps(const std::tm& time, std::time_t& t1, std::time_t& t2);

    double bilinearInterpolate(const std::vector<std::vector<double>>& data,
                               double lat, double lon);

    int latToIndex(double lat) const;
    int lonToIndex(double lon) const;
    double indexToLat(int idx) const;
    double indexToLon(int idx) const;
};
