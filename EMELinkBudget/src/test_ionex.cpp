#include "IonexReader.h"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "IONEX Reader Test Program\n";
    std::cout << "=========================\n\n";

    IonexReader reader("../data/data.txt");

    if (!reader.isOpen()) {
        std::cerr << "Error: Could not open data.txt\n";
        std::cerr << "Please ensure the file exists in the current directory.\n";
        return 1;
    }

    std::cout << "File opened successfully!\n\n";

    const IonexHeader& header = reader.getHeader();

    std::cout << "IONEX Header Information:\n";
    std::cout << "-------------------------\n";
    std::cout << "Version: " << header.version << "\n";
    std::cout << "Number of maps: " << header.numMaps << "\n";
    std::cout << "Interval: " << header.interval << " seconds\n";
    std::cout << "Latitude range: " << header.lat1 << " to " << header.lat2
              << " (step: " << header.dlat << ")\n";
    std::cout << "Longitude range: " << header.lon1 << " to " << header.lon2
              << " (step: " << header.dlon << ")\n";
    std::cout << "Grid size: " << header.numLat << " x " << header.numLon << "\n";
    std::cout << "Exponent: " << header.exponent << "\n\n";

    std::cout << "Testing TEC value retrieval:\n";
    std::cout << "----------------------------\n";

    std::tm testTime = {};
    testTime.tm_year = 2026 - 1900;
    testTime.tm_mon = 2 - 1;
    testTime.tm_mday = 9;
    testTime.tm_hour = 0;
    testTime.tm_min = 0;
    testTime.tm_sec = 0;

    struct TestLocation {
        std::string name;
        double lat;
        double lon;
    };

    TestLocation locations[] = {
        {"Hefei (OM81ks)", 31.79, 116.87},
        {"Moscow (KO93bs)", 53.77, 38.13},
        {"Grid point (30N, 115E)", 30.0, 115.0},
        {"Grid point (50N, 40E)", 50.0, 40.0}
    };

    for (const auto& loc : locations) {
        double vtec;
        bool success = reader.getTecValueInterpolated(testTime, loc.lat, loc.lon, vtec);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << loc.name << " (" << loc.lat << "N, " << loc.lon << "E): ";

        if (success) {
            std::cout << vtec << " TECU\n";
        } else {
            std::cout << "No data available\n";
        }
    }

    std::cout << "\nTesting time interpolation:\n";
    std::cout << "---------------------------\n";

    std::tm testTime2 = testTime;
    testTime2.tm_hour = 1;
    testTime2.tm_min = 30;

    double vtec;
    if (reader.getTecValueInterpolated(testTime2, 31.79, 116.87, vtec)) {
        std::cout << "Hefei at 01:30 UTC: " << vtec << " TECU (interpolated)\n";
    } else {
        std::cout << "Could not interpolate value\n";
    }

    std::cout << "\nTest complete!\n";

    return 0;
}
