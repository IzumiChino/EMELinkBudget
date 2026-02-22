#pragma once

#include "LinkBudgetTypes.h"
#include "Parameters.h"
#include "SpectralSpreadingCalculator.h"
#include <cmath>
#include <ctime>

// ========== Geometry Calculator ==========
class GeometryCalculator {
public:
    GeometryCalculator();

    GeometryResults calculate(
        const SiteParameters& txSite,
        const SiteParameters& rxSite,
        const MoonEphemeris& moonEphem,
        std::time_t observationTime,
        double frequency_MHz = 432.0);

    void calculateMoonPosition(
        double latitude, double longitude,
        double moonRA, double moonDEC,
        double hourAngle,
        double& azimuth, double& elevation);

    double calculateDistance(
        double stationLat, double stationLon,
        double moonRA, double moonDEC,
        double moonDistance_km);

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

    double calculateDopplerShift(
        double frequency_MHz,
        double velocity_TX_km_s,
        double velocity_RX_km_s);

    double estimateRadialVelocity(
        const SiteParameters& site,
        const MoonEphemeris& moonEphem,
        std::time_t observationTime,
        double deltaTime_s = 60.0);

private:
    static constexpr double SPEED_OF_LIGHT_KM_S = 299792.458;
};
