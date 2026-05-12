#include "NOAAGlotecReader.h"
#include <iostream>
#include <iomanip>
#include <ctime>

int main() {
    NOAAGlotecReader reader;

    std::cout << "NOAA GLOTEC TEC Data Reader Test\n";
    std::cout << "==================================\n\n";

    std::tm testTime = {};
    testTime.tm_year = 2026 - 1900;
    testTime.tm_mon = 1;
    testTime.tm_mday = 9;
    testTime.tm_hour = 14;
    testTime.tm_min = 17;
    testTime.tm_sec = 0;

    std::cout << "Test time: 2026-02-09 14:17:00 UTC\n";
    std::cout << "Expected URL: " << reader.getDataUrl(testTime) << "\n\n";

    std::cout << "Fetching TEC data from NOAA...\n";
    GlotecData data;

    if (reader.fetchTecData(testTime, data)) {
        std::cout << "Success! Data retrieved.\n\n";
        std::cout << "Grid info:\n";
        std::cout << "  Longitude: " << data.lonStart << " to "
                  << (data.lonStart + (data.numLon - 1) * data.lonStep)
                  << " (step: " << data.lonStep << ")\n";
        std::cout << "  Latitude: " << data.latStart << " to "
                  << (data.latStart + (data.numLat - 1) * data.latStep)
                  << " (step: " << data.latStep << ")\n";
        std::cout << "  Total points: " << data.tecValues.size() << "\n\n";

        std::cout << "Testing TEC interpolation at specific locations:\n";

        struct TestLocation {
            const char* name;
            double lat;
            double lon;
        };

        TestLocation locations[] = {
            {"New York (FN30)", 40.7, -74.0},
            {"London (IO91)", 51.5, -0.1},
            {"Tokyo (PM95)", 35.7, 139.7},
            {"Sydney (QF56)", -33.9, 151.2}
        };

        for (const auto& loc : locations) {
            double tec;
            if (reader.getTecAtLocation(data, loc.lat, loc.lon, tec)) {
                std::cout << "  " << std::setw(20) << std::left << loc.name
                          << ": " << std::fixed << std::setprecision(2)
                          << tec << " TECU\n";
            }
        }

    } else {
        std::cout << "Failed to fetch data.\n";
        std::cout << "Note: This test requires internet connection and NOAA server availability.\n";
    }

    std::cout << "\nTest completed.\n";
    return 0;
}
