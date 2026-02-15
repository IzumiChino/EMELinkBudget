#include "GeometryCalculator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== GeometryCalculator Implementation ==========

GeometryCalculator::GeometryCalculator() {
}

double GeometryCalculator::deg2rad(double degrees) const {
    return degrees * M_PI / 180.0;
}

double GeometryCalculator::rad2deg(double radians) const {
    return radians * 180.0 / M_PI;
}

double GeometryCalculator::normalizeAngle(double angle) const {
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle < -M_PI) angle += 2.0 * M_PI;
    return angle;
}

void GeometryCalculator::calculateMoonPosition(
    double latitude, double longitude,
    double moonRA, double moonDEC,
    double hourAngle,
    double& azimuth, double& elevation) {

    double sinLat = std::sin(latitude);
    double cosLat = std::cos(latitude);
    double sinDec = std::sin(moonDEC);
    double cosDec = std::cos(moonDEC);
    double cosH = std::cos(hourAngle);
    double sinH = std::sin(hourAngle);

    // Calculate elevation
    elevation = std::asin(sinLat * sinDec + cosLat * cosDec * cosH);

    // Calculate azimuth
    double tanDec = std::tan(moonDEC);
    azimuth = std::atan2(sinH, cosH * sinLat - tanDec * cosLat);

    // Normalize azimuth to [0, 2π]
    if (azimuth < 0) {
        azimuth += 2.0 * M_PI;
    }
}

double GeometryCalculator::calculateDistance(
    double stationLat, double stationLon,
    double moonRA, double moonDEC,
    double moonDistance_km) {

    // For EME, the distance variation due to station location is negligible
    // compared to the moon distance (~384,400 km vs ~6,371 km Earth radius)
    // We use the geocentric distance as approximation
    return moonDistance_km;
}

double GeometryCalculator::calculateHourAngle(
    double longitude,
    double moonRA,
    std::time_t observationTime) {

    // Calculate Local Sidereal Time (LST)
    // Simplified calculation - for production use, implement full GMST calculation

    // Julian Date
    double JD = 2440587.5 + (observationTime / 86400.0);

    // Days since J2000.0
    double T = (JD - 2451545.0) / 36525.0;

    // Greenwich Mean Sidereal Time at 0h UT (degrees)
    double GMST = 280.46061837 + 360.98564736629 * (JD - 2451545.0)
                  + 0.000387933 * T * T
                  - T * T * T / 38710000.0;

    // Normalize to [0, 360)
    GMST = std::fmod(GMST, 360.0);
    if (GMST < 0) GMST += 360.0;

    // Local Sidereal Time
    double LST_deg = GMST + rad2deg(longitude);
    LST_deg = std::fmod(LST_deg, 360.0);
    if (LST_deg < 0) LST_deg += 360.0;

    // Hour Angle = LST - RA
    double HA_deg = LST_deg - rad2deg(moonRA);

    // Normalize to [-180, 180]
    while (HA_deg > 180.0) HA_deg -= 360.0;
    while (HA_deg < -180.0) HA_deg += 360.0;

    return deg2rad(HA_deg);
}

GeometryResults GeometryCalculator::calculate(
    const SiteParameters& txSite,
    const SiteParameters& rxSite,
    const MoonEphemeris& moonEphem,
    std::time_t observationTime) {

    GeometryResults results;

    // Use provided moon ephemeris data
    results.moonRA_deg = rad2deg(moonEphem.rightAscension);
    results.moonDEC_deg = rad2deg(moonEphem.declination);
    results.moonDistance_km = moonEphem.distance_km;
    results.ephemerisSource = moonEphem.ephemerisSource;

    // Calculate hour angles if not provided
    double hourAngle_TX = moonEphem.hourAngle_DX;
    double hourAngle_RX = moonEphem.hourAngle_Home;

    if (hourAngle_TX == 0.0 && hourAngle_RX == 0.0) {
        hourAngle_TX = calculateHourAngle(
            txSite.longitude,
            moonEphem.rightAscension,
            observationTime);

        hourAngle_RX = calculateHourAngle(
            rxSite.longitude,
            moonEphem.rightAscension,
            observationTime);
    }

    // Store hour angles in results
    results.hourAngle_TX_rad = hourAngle_TX;
    results.hourAngle_RX_rad = hourAngle_RX;

    // Calculate moon position for TX station
    calculateMoonPosition(
        txSite.latitude, txSite.longitude,
        moonEphem.rightAscension, moonEphem.declination,
        hourAngle_TX,
        results.moonAzimuth_TX_deg, results.moonElevation_TX_deg);

    results.moonAzimuth_TX_deg = rad2deg(results.moonAzimuth_TX_deg);
    results.moonElevation_TX_deg = rad2deg(results.moonElevation_TX_deg);

    // Calculate moon position for RX station
    calculateMoonPosition(
        rxSite.latitude, rxSite.longitude,
        moonEphem.rightAscension, moonEphem.declination,
        hourAngle_RX,
        results.moonAzimuth_RX_deg, results.moonElevation_RX_deg);

    results.moonAzimuth_RX_deg = rad2deg(results.moonAzimuth_RX_deg);
    results.moonElevation_RX_deg = rad2deg(results.moonElevation_RX_deg);

    // Calculate distances
    results.distance_TX_km = calculateDistance(
        txSite.latitude, txSite.longitude,
        moonEphem.rightAscension, moonEphem.declination,
        moonEphem.distance_km);

    results.distance_RX_km = calculateDistance(
        rxSite.latitude, rxSite.longitude,
        moonEphem.rightAscension, moonEphem.declination,
        moonEphem.distance_km);

    results.totalPathLength_km = results.distance_TX_km + results.distance_RX_km;

    // Doppler shift calculation (placeholder - will be implemented in DopplerCalculator)
    results.dopplerShift_Hz = 0.0;

    return results;
}

// ========== DopplerCalculator Implementation ==========

DopplerCalculator::DopplerCalculator() {
}

double DopplerCalculator::calculateDopplerShift(
    double frequency_MHz,
    double velocity_TX_km_s,
    double velocity_RX_km_s) {

    // Total radial velocity (positive = approaching, negative = receding)
    double totalVelocity_km_s = velocity_TX_km_s + velocity_RX_km_s;

    // Doppler shift: Δf = -f₀ * (v/c)
    // Negative sign because approaching moon increases frequency
    double dopplerShift_Hz = -(frequency_MHz * 1e6) * (totalVelocity_km_s / SPEED_OF_LIGHT_KM_S);

    return dopplerShift_Hz;
}

double DopplerCalculator::estimateRadialVelocity(
    const SiteParameters& site,
    const MoonEphemeris& moonEphem,
    std::time_t observationTime,
    double deltaTime_s) {

    // Simplified radial velocity estimation
    // For accurate calculation, need moon position at t and t+Δt

    // Placeholder: return 0 for now
    // In production, this should calculate the rate of change of distance
    return 0.0;
}
