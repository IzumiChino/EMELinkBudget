#pragma once

#include "LinkBudgetTypes.h"
#include <cmath>

// ========== Path Loss Calculator ==========
class PathLossCalculator {
public:
    PathLossCalculator();

    PathLossResults calculate(
        double frequency_MHz,
        double distance_TX_km,
        double distance_RX_km,
        double elevation_TX_deg,
        double elevation_RX_deg,
        bool includeAtmospheric = true,
        bool useHagforsModel = true);

    double calculateFreeSpaceLoss(
        double frequency_MHz,
        double distance_km);

    double calculateLunarScatteringLoss(
        double reflectivity = 0.07);

    // Hagfors' Law lunar scattering model
    double calculateLunarScatteringLossHagfors(
        double frequency_MHz,
        double bistaticAngle_deg,
        double& rcs_dBsm,
        double& roughnessParam);

    double calculateAtmosphericLoss(
        double frequency_MHz,
        double elevation_deg);

    // Calculate bistatic angle from TX and RX elevations
    double calculateBistaticAngle(
        double elevation_TX_deg,
        double elevation_RX_deg,
        double distance_TX_km,
        double distance_RX_km);

private:
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;
    static constexpr double MOON_RADIUS_KM = 1737.1;
    static constexpr double PI = 3.14159265358979323846;

    double deg2rad(double degrees) const;

    // Hagfors model helper functions
    double calculateHagforsRoughnessParameter(double frequency_MHz);
    double calculateHagforsScatteringCrossSection(
        double bistaticAngle_rad,
        double roughnessParam);
};

// ========== Atmospheric Model ==========
class AtmosphericModel {
public:
    AtmosphericModel();

    double getZenithAttenuation(double frequency_MHz);

    double getSlantAttenuation(
        double frequency_MHz,
        double elevation_deg);

private:
    double calculateGaseousAttenuation(double frequency_MHz);
};
