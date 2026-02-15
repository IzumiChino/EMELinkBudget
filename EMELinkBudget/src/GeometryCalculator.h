#pragma once

#include "LinkBudgetTypes.h"
#include "Parameters.h"
#include <cmath>
#include <ctime>

// ========== Geometry Calculator ==========
class GeometryCalculator {
public:
    GeometryCalculator();

    // Calculate moon position and distances
    GeometryResults calculate(
        const SiteParameters& txSite,
        const SiteParameters& rxSite,
        const MoonEphemeris& moonEphem,
        std::time_t observationTime);

    // Calculate moon elevation and azimuth from RA/DEC
    void calculateMoonPosition(
        double latitude, double longitude,
        double moonRA, double moonDEC,
        double hourAngle,
        double& azimuth, double& elevation);

    // Calculate distance from station to moon
    double calculateDistance(
        double stationLat, double stationLon,
        double moonRA, double moonDEC,
        double moonDistance_km);

    // Calculate hour angle from observation time
    double calculateHourAngle(
        double longitude,
        double moonRA,
        std::time_t observationTime);

private:
    double deg2rad(double degrees) const;
    double rad2deg(double radians) const;
    double normalizeAngle(double angle) const;
};

// ========== Doppler Calculator ==========
class DopplerCalculator {
public:
    DopplerCalculator();

    // Calculate Doppler shift
    double calculateDopplerShift(
        double frequency_MHz,
        double velocity_TX_km_s,
        double velocity_RX_km_s);

    // Estimate radial velocity from moon ephemeris
    double estimateRadialVelocity(
        const SiteParameters& site,
        const MoonEphemeris& moonEphem,
        std::time_t observationTime,
        double deltaTime_s = 60.0);

private:
    static constexpr double SPEED_OF_LIGHT_KM_S = 299792.458;
};
