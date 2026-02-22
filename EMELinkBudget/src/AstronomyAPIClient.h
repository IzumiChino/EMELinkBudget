#pragma once

#include <string>
#include <ctime>

// ========== Astronomy API Client ==========

class AstronomyAPIClient {
public:
struct MoonData {
    double ra_deg;
    double dec_deg;
    double distance_km;
    double azimuth_deg;
    double elevation_deg;
    double range_rate_km_s;
    double libration_lon_deg;
    double libration_lat_deg;
    double libration_lon_rate_deg_day;
    double libration_lat_rate_deg_day;
    std::string source;
    bool valid;

        MoonData() : ra_deg(0), dec_deg(0), distance_km(0),
                     azimuth_deg(0), elevation_deg(0),
                     range_rate_km_s(0), libration_lon_deg(0), libration_lat_deg(0),
                     libration_lon_rate_deg_day(0), libration_lat_rate_deg_day(0),
                     source(""), valid(false) {}
    };

    AstronomyAPIClient();

    bool fetchMoonPosition(
        std::time_t observationTime,
        double observerLat_deg,
        double observerLon_deg,
        MoonData& result);

    std::string getLastError() const { return m_lastError; }

private:
std::string m_lastError;

std::string buildAPIUrl(
        std::time_t time,
        double lat,
        double lon);

    bool parseResponse(
        const std::string& response,
        MoonData& result);

    bool extractJsonValue(
        const std::string& json,
        const std::string& key,
        std::string& value);

    bool extractNestedValue(
        const std::string& json,
        const std::string& path,
        double& value);

    std::string formatTime(std::time_t time);
};
