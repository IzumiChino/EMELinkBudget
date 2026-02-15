#include "MaidenheadGrid.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

void printSeparator(char c = '=', int length = 75) {
    std::cout << std::string(length, c) << std::endl;
}

void testGridToLatLon() {
    std::cout << "\n=== Test 1: Grid to Lat/Lon Conversion ===" << std::endl;

    struct TestCase {
        std::string grid;
        double expected_lat;
        double expected_lon;
        std::string location;
    };

    std::vector<TestCase> tests = {
        {"FN20xa", 40.02, -74.04, "New York area"},
        {"PM95vr", 35.73, 139.79, "Tokyo area"},
        {"JO01", 51.5, 1.0, "UK area (4-char)"},
        {"IO91", 51.5, -1.0, "UK area (4-char)"},
        {"OM81ks", 31.79, 116.87, "Hefei, Anhui, China"},
        {"KO93bs", 53.77, 38.13, "Moscow area, Russia"}
    };

    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::setw(10) << "Grid"
              << std::setw(12) << "Latitude"
              << std::setw(12) << "Longitude"
              << "  Location" << std::endl;
    printSeparator('-', 75);

    for (const auto& test : tests) {
        double lat, lon;
        try {
            MaidenheadGrid::gridToLatLon(test.grid, lat, lon);
            std::cout << std::setw(10) << test.grid
                      << std::setw(12) << lat
                      << std::setw(12) << lon
                      << "  " << test.location << std::endl;

            double tolerance = (test.grid.length() == 4) ? 0.1 : 0.05;
            if (std::abs(lat - test.expected_lat) > tolerance ||
                std::abs(lon - test.expected_lon) > tolerance) {
                std::cout << "  WARNING: Expected " << test.expected_lat
                          << ", " << test.expected_lon << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "  ERROR: " << e.what() << std::endl;
        }
    }
}

void testLatLonToGrid() {
    std::cout << "\n=== Test 2: Lat/Lon to Grid Conversion ===" << std::endl;

    struct TestCase {
        double lat;
        double lon;
        std::string expected_grid;
        std::string location;
    };

    std::vector<TestCase> tests = {
        {40.7128, -74.0060, "FN20xr", "New York City"},
        {51.5074, -0.1278, "IO91wm", "London"},
        {35.6762, 139.6503, "PM95sq", "Tokyo"},
        {48.8566, 2.3522, "JN18eu", "Paris"},
        {-33.8688, 151.2093, "QF56od", "Sydney"}
    };

    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::setw(12) << "Latitude"
              << std::setw(12) << "Longitude"
              << std::setw(12) << "Grid"
              << std::setw(12) << "Expected"
              << "  Location" << std::endl;
    printSeparator('-', 75);

    for (const auto& test : tests) {
        try {
            std::string grid = MaidenheadGrid::latLonToGrid(test.lat, test.lon, 6);
            std::cout << std::setw(12) << test.lat
                      << std::setw(12) << test.lon
                      << std::setw(12) << grid
                      << std::setw(12) << test.expected_grid
                      << "  " << test.location;

            if (grid == test.expected_grid) {
                std::cout << " [OK]" << std::endl;
            } else {
                std::cout << " [FAIL]" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "  ERROR: " << e.what() << std::endl;
        }
    }
}

void testRoundTrip() {
    std::cout << "\n=== Test 3: Round-Trip Conversion ===" << std::endl;
    std::cout << "Converting Grid -> Lat/Lon -> Grid should return same grid" << std::endl;

    std::vector<std::string> grids = {
        "FN20xa", "PM95vr", "JO01bh", "IO91wm", "OM81ks", "KO93bs"
    };

    std::cout << std::setw(12) << "Original"
              << std::setw(12) << "Converted"
              << "  Status" << std::endl;
    printSeparator('-', 75);

    for (const auto& original : grids) {
        double lat, lon;
        MaidenheadGrid::gridToLatLon(original, lat, lon);
        std::string converted = MaidenheadGrid::latLonToGrid(lat, lon, static_cast<int>(original.length()));

        std::cout << std::setw(12) << original
                  << std::setw(12) << converted;

        if (original == converted) {
            std::cout << "  PASS" << std::endl;
        } else {
            std::cout << "  FAIL" << std::endl;
        }
    }
}

void testDistance() {
    std::cout << "\n=== Test 4: Distance Calculation ===" << std::endl;

    struct TestCase {
        std::string grid1;
        std::string grid2;
        double expected_km;
        std::string description;
    };

    std::vector<TestCase> tests = {
        {"FN20xa", "PM95vr", 10908, "New York to Tokyo"},
        {"JO01", "IO91", 138, "UK grid to grid"},
        {"OM81ks", "KO93bs", 5850, "Hefei to Moscow"},
        {"FN20xa", "FN20xa", 0, "Same location"}
    };

    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::setw(10) << "Grid 1"
              << std::setw(10) << "Grid 2"
              << std::setw(15) << "Distance (km)"
              << std::setw(15) << "Expected (km)"
              << "  Description" << std::endl;
    printSeparator('-', 75);

    for (const auto& test : tests) {
        double distance = MaidenheadGrid::calculateDistance(test.grid1, test.grid2);
        std::cout << std::setw(10) << test.grid1
                  << std::setw(10) << test.grid2
                  << std::setw(15) << distance
                  << std::setw(15) << test.expected_km
                  << "  " << test.description;

        double error = std::abs(distance - test.expected_km) / (test.expected_km + 0.001) * 100.0;
        if (error < 1.0 || test.expected_km == 0) {
            std::cout << " [OK]" << std::endl;
        } else {
            std::cout << " [FAIL: " << error << "%]" << std::endl;
        }
    }
}

void interactiveTest() {
    std::cout << "\n=== Interactive Test ===" << std::endl;
    std::cout << "Enter a Maidenhead Grid locator to test: ";
    std::string grid;
    std::cin >> grid;

    try {
        double lat, lon;
        MaidenheadGrid::gridToLatLon(grid, lat, lon);

        std::cout << "\nResults for " << grid << ":" << std::endl;
        std::cout << "  Latitude:  " << std::fixed << std::setprecision(6) << lat << " deg" << std::endl;
        std::cout << "  Longitude: " << lon << " deg" << std::endl;

        std::string grid_back = MaidenheadGrid::latLonToGrid(lat, lon, static_cast<int>(grid.length()));
        std::cout << "  Round-trip: " << grid_back;
        if (grid == grid_back) {
            std::cout << " [Correct]" << std::endl;
        } else {
            std::cout << " [Incorrect]" << std::endl;
        }

        std::cout << "\nAlternative formats:" << std::endl;
        std::cout << "  4-char: " << MaidenheadGrid::latLonToGrid(lat, lon, 4) << std::endl;
        std::cout << "  6-char: " << MaidenheadGrid::latLonToGrid(lat, lon, 6) << std::endl;

    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
    }
}

int main() {
    printSeparator();
    std::cout << "  Maidenhead Grid Locator System - Test Suite" << std::endl;
    printSeparator();

    std::cout << "\nThis program tests the Maidenhead Grid conversion functions." << std::endl;
    std::cout << "You can verify results at: https://www.levinecentral.com/ham/grid_square.php" << std::endl;

    testGridToLatLon();
    testLatLonToGrid();
    testRoundTrip();
    testDistance();
    interactiveTest();

    std::cout << "\n";
    printSeparator();
    std::cout << "Test suite complete!" << std::endl;
    printSeparator();

    std::cout << "\nPress Enter to exit...";
    std::cin.ignore();
    std::cin.get();

    return 0;
}
