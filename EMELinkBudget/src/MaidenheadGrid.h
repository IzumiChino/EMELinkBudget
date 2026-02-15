#pragma once

#include <string>
#include <stdexcept>
#include <cctype>
#include <cmath>

// ========== Maidenhead Grid Locator Converter ==========
class MaidenheadGrid {
public:
    static void gridToLatLon(const std::string& grid, double& latitude, double& longitude) {
        if (grid.length() != 4 && grid.length() != 6) {
            throw std::invalid_argument("Grid locator must be 4 or 6 characters");
        }

        std::string gridUpper = grid;
        for (char& c : gridUpper) {
            c = std::toupper(c);
        }

        if (!std::isalpha(gridUpper[0]) || !std::isalpha(gridUpper[1]) ||
            !std::isdigit(gridUpper[2]) || !std::isdigit(gridUpper[3])) {
            throw std::invalid_argument("Invalid grid locator format");
        }

        int field_lon = gridUpper[0] - 'A';
        int field_lat = gridUpper[1] - 'A';
        int square_lon = gridUpper[2] - '0';
        int square_lat = gridUpper[3] - '0';

        longitude = -180.0 + field_lon * 20.0 + square_lon * 2.0;
        latitude = -90.0 + field_lat * 10.0 + square_lat * 1.0;

        if (grid.length() == 6) {
            if (!std::isalpha(grid[4]) || !std::isalpha(grid[5])) {
                throw std::invalid_argument("Invalid subsquare format");
            }

            char subsq_lon = std::tolower(grid[4]);
            char subsq_lat = std::tolower(grid[5]);

            if (subsq_lon < 'a' || subsq_lon > 'x' || subsq_lat < 'a' || subsq_lat > 'x') {
                throw std::invalid_argument("Subsquare must be a-x");
            }

            int sub_lon = subsq_lon - 'a';
            int sub_lat = subsq_lat - 'a';

            longitude += sub_lon * (2.0 / 24.0) + (1.0 / 24.0);
            latitude += sub_lat * (1.0 / 24.0) + (1.0 / 48.0);
        } else {
            longitude += 1.0;
            latitude += 0.5;
        }
    }

    static std::string latLonToGrid(double latitude, double longitude, int precision = 6) {
        if (latitude < -90.0 || latitude > 90.0) {
            throw std::invalid_argument("Latitude must be between -90 and 90");
        }
        if (longitude < -180.0 || longitude > 180.0) {
            throw std::invalid_argument("Longitude must be between -180 and 180");
        }

        std::string grid;

        double lon = longitude + 180.0;
        double lat = latitude + 90.0;

        int field_lon = static_cast<int>(lon / 20.0);
        int field_lat = static_cast<int>(lat / 10.0);
        grid += static_cast<char>('A' + field_lon);
        grid += static_cast<char>('A' + field_lat);

        lon -= field_lon * 20.0;
        lat -= field_lat * 10.0;
        int square_lon = static_cast<int>(lon / 2.0);
        int square_lat = static_cast<int>(lat / 1.0);
        grid += static_cast<char>('0' + square_lon);
        grid += static_cast<char>('0' + square_lat);

        if (precision >= 6) {
            lon -= square_lon * 2.0;
            lat -= square_lat * 1.0;
            int sub_lon = static_cast<int>(lon / (2.0 / 24.0));
            int sub_lat = static_cast<int>(lat / (1.0 / 24.0));
            grid += static_cast<char>('a' + sub_lon);
            grid += static_cast<char>('a' + sub_lat);
        }

        return grid;
    }

    static double calculateDistance(const std::string& grid1, const std::string& grid2) {
        double lat1, lon1, lat2, lon2;
        gridToLatLon(grid1, lat1, lon1);
        gridToLatLon(grid2, lat2, lon2);
        return calculateDistanceLatLon(lat1, lon1, lat2, lon2);
    }

    static double calculateDistanceLatLon(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371.0;
        const double PI = 3.14159265358979323846;

        lat1 = lat1 * PI / 180.0;
        lon1 = lon1 * PI / 180.0;
        lat2 = lat2 * PI / 180.0;
        lon2 = lon2 * PI / 180.0;

        double dlat = lat2 - lat1;
        double dlon = lon2 - lon1;

        double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dlon / 2.0) * std::sin(dlon / 2.0);

        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

        return R * c;
    }
};
