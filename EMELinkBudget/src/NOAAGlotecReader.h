#pragma once

#include <string>
#include <vector>
#include <ctime>

struct GlotecData {
    std::vector<float> tecValues;
    int numLon;
    int numLat;
    double lonStart;
    double latStart;
    double lonStep;
    double latStep;
    std::tm timestamp;
    bool isValid;

    GlotecData() : numLon(72), numLat(0), lonStart(-177.5), latStart(-88.75),
                   lonStep(5.0), latStep(2.5), isValid(false) {
        timestamp = {};
    }
};

class NOAAGlotecReader {
public:
    NOAAGlotecReader();

    bool fetchTecData(const std::tm& requestTime, GlotecData& data);

    bool getTecAtLocation(const GlotecData& data, double lat, double lon, double& tec);

    std::string getDataUrl(const std::tm& time) const;

private:
    std::string m_baseUrl;

    std::tm roundToNearest5Minutes(const std::tm& time, bool roundDown) const;

    bool parseGeoJson(const std::string& jsonContent, GlotecData& data);

    double bilinearInterpolate(const GlotecData& data, double lat, double lon) const;

    int getGridIndex(int col, int row, int numCols) const;
};
