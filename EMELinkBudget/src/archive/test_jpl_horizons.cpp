#include "AstronomyAPIClient.h"
#include <iostream>
#include <iomanip>
#include <ctime>

int main() {
    std::cout << "========================================" << '\n';
    std::cout << "  JPL Horizons API Test" << '\n';
    std::cout << "========================================\n" << '\n';

    // Test parameters
    std::time_t testTime = std::time(nullptr);
    double lat = 31.77;   // BI6DX latitude
    double lon = 116.87;  // BI6DX longitude

    std::cout << "Test Parameters:" << '\n';
    std::cout << "  Time: " << std::ctime(&testTime);
    std::cout << "  Location: " << std::fixed << std::setprecision(2)
              << lat << "N, " << lon << "E" << '\n';
    std::cout << '\n';

    // Create API client
    AstronomyAPIClient client;
    AstronomyAPIClient::MoonData moonData;

    std::cout << "Fetching moon position from JPL Horizons..." << '\n';

    // Fetch data
    if (client.fetchMoonPosition(testTime, lat, lon, moonData)) {
        std::cout << "\n[SUCCESS] Moon position retrieved!\n" << '\n';

        std::cout << "Moon Position Data:" << '\n';
        std::cout << "  Right Ascension: " << std::fixed << std::setprecision(4)
                  << moonData.ra_deg << " deg" << '\n';
        std::cout << "  Declination:     " << moonData.dec_deg << " deg" << '\n';
        std::cout << "  Distance:        " << std::setprecision(1)
                  << moonData.distance_km << " km" << '\n';
        std::cout << "  Source:          " << moonData.source << '\n';

        // Validate ranges
        std::cout << "\nValidation:" << '\n';
        std::cout << "  RA range (0-360):       "
                  << (moonData.ra_deg >= 0 && moonData.ra_deg <= 360 ? "OK" : "FAIL")
                  << '\n';
        std::cout << "  DEC range (-90 to 90):  "
                  << (moonData.dec_deg >= -90 && moonData.dec_deg <= 90 ? "OK" : "FAIL")
                  << '\n';
        std::cout << "  Distance (356k-406k):   "
                  << (moonData.distance_km >= 356000 && moonData.distance_km <= 406000 ? "OK" : "FAIL")
                  << '\n';

    } else {
        std::cout << "\n[FAILED] Could not retrieve moon position" << '\n';
        std::cout << "Error: " << client.getLastError() << '\n';
        return 1;
    }

    std::cout << "\n========================================" << '\n';
    std::cout << "Test completed successfully!" << '\n';
    std::cout << "========================================" << '\n';

    return 0;
}
