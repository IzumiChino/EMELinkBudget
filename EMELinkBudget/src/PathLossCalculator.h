#pragma once

#include "LinkBudgetTypes.h"
#include <cmath>

// ========== Path Loss Calculator ==========
class PathLossCalculator {
public:
    PathLossCalculator();

    // Calculate all path losses
    PathLossResults calculate(
        double frequency_MHz,
        double distance_TX_km,
        double distance_RX_km,
        double elevation_TX_deg,
        double elevation_RX_deg,
        bool includeAtmospheric = true);

    // Individual loss calculations
    double calculateFreeSpaceLoss(
        double frequency_MHz,
        double distance_km);

    double calculateLunarScatteringLoss(
        double reflectivity = 0.07);

    double calculateAtmosphericLoss(
        double frequency_MHz,
        double elevation_deg);

private:
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;
    static constexpr double MOON_RADIUS_KM = 1737.1;
    static constexpr double PI = 3.14159265358979323846;

    double deg2rad(double degrees) const;
};

// ========== Atmospheric Model ==========
class AtmosphericModel {
public:
    AtmosphericModel();

    // Calculate zenith attenuation for given frequency
    double getZenithAttenuation(double frequency_MHz);

    // Calculate slant path attenuation
    double getSlantAttenuation(
        double frequency_MHz,
        double elevation_deg);

private:
    // Frequency-dependent attenuation model
    double calculateGaseousAttenuation(double frequency_MHz);
};
