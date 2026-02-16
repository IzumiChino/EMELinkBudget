#include "AstronomyAPIClient.h"
#include <iostream>
#include <iomanip>
#include <ctime>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  JPL Horizons API Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Test parameters
    std::time_t testTime = std::time(nullptr);
    double lat = 31.77;   // BI6DX latitude
    double lon = 116.87;  // BI6DX longitude

    std::cout << "Test Parameters:" << std::endl;
    std::cout << "  Time: " << std::ctime(&testTime);
    std::cout << "  Location: " << std::fixed << std::setprecision(2)
              << lat << "N, " << lon << "E" << std::endl;
    std::cout << std::endl;

    // Create API client
    AstronomyAPIClient client;
    AstronomyAPIClient::MoonData moonData;

    std::cout << "Fetching moon position from JPL Horizons..." << std::endl;

    // Fetch data
    if (client.fetchMoonPosition(testTime, lat, lon, moonData)) {
        std::cout << "\n[SUCCESS] Moon position retrieved!\n" << std::endl;

        std::cout << "Moon Position Data:" << std::endl;
        std::cout << "  Right Ascension: " << std::fixed << std::setprecision(4)
                  << moonData.ra_deg << " deg" << std::endl;
        std::cout << "  Declination:     " << moonData.dec_deg << " deg" << std::endl;
        std::cout << "  Distance:        " << std::setprecision(1)
                  << moonData.distance_km << " km" << std::endl;
        std::cout << "  Source:          " << moonData.source << std::endl;

        // Validate ranges
        std::cout << "\nValidation:" << std::endl;
        std::cout << "  RA range (0-360):       "
                  << (moonData.ra_deg >= 0 && moonData.ra_deg <= 360 ? "OK" : "FAIL")
                  << std::endl;
        std::cout << "  DEC range (-90 to 90):  "
                  << (moonData.dec_deg >= -90 && moonData.dec_deg <= 90 ? "OK" : "FAIL")
                  << std::endl;
        std::cout << "  Distance (356k-406k):   "
                  << (moonData.distance_km >= 356000 && moonData.distance_km <= 406000 ? "OK" : "FAIL")
                  << std::endl;

    } else {
        std::cout << "\n[FAILED] Could not retrieve moon position" << std::endl;
        std::cout << "Error: " << client.getLastError() << std::endl;
        return 1;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Test completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
