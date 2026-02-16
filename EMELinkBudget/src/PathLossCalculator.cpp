#include "PathLossCalculator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== PathLossCalculator Implementation ==========

PathLossCalculator::PathLossCalculator() {
}

double PathLossCalculator::deg2rad(double degrees) const {
    return degrees * M_PI / 180.0;
}

double PathLossCalculator::calculateFreeSpaceLoss(
    double frequency_MHz,
    double distance_km) {

    double frequency_Hz = frequency_MHz * 1e6;
    double distance_m = distance_km * 1000.0;

    double wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    double loss_dB = 20.0 * std::log10((4.0 * M_PI * distance_m) / wavelength_m);

    return loss_dB;
}

double PathLossCalculator::calculateLunarScatteringLoss(double reflectivity) {
double geometricArea_m2 = M_PI * (MOON_RADIUS_KM * 1000.0) * (MOON_RADIUS_KM * 1000.0);
    double sigma_m2 = reflectivity * geometricArea_m2;

    double moonGain_dB = 10.0 * std::log10(sigma_m2);

    return -moonGain_dB;
}

double PathLossCalculator::calculateAtmosphericLoss(
    double frequency_MHz,
    double elevation_deg) {

    if (elevation_deg < 0) {
        return 0.0;
    }

    AtmosphericModel atmModel;
    return atmModel.getSlantAttenuation(frequency_MHz, elevation_deg);
}

PathLossResults PathLossCalculator::calculate(
    double frequency_MHz,
    double distance_TX_km,
    double distance_RX_km,
    double elevation_TX_deg,
    double elevation_RX_deg,
    bool includeAtmospheric) {

    PathLossResults results;

    double frequency_Hz = frequency_MHz * 1e6;
    results.wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    double distance_km = (distance_TX_km + distance_RX_km) / 2.0;

    double L_echo = 20.0 * std::log10(frequency_MHz) +
                    40.0 * std::log10(distance_km) -
                    14.6;

    results.freeSpaceLoss_dB = L_echo;

    results.lunarReflectivity = 0.07;
    results.lunarScatteringLoss_dB = calculateLunarScatteringLoss(results.lunarReflectivity);

    // Atmospheric loss
    if (includeAtmospheric) {
        results.atmosphericLoss_TX_dB = calculateAtmosphericLoss(
            frequency_MHz, elevation_TX_deg);
        results.atmosphericLoss_RX_dB = calculateAtmosphericLoss(
            frequency_MHz, elevation_RX_deg);
        results.atmosphericLoss_Total_dB =
            results.atmosphericLoss_TX_dB + results.atmosphericLoss_RX_dB;
    } else {
        results.atmosphericLoss_TX_dB = 0.0;
        results.atmosphericLoss_RX_dB = 0.0;
        results.atmosphericLoss_Total_dB = 0.0;
    }

    // Total path loss = EME echo loss + atmospheric loss
    // (Lunar scattering is already in the echo formula)
    results.totalPathLoss_dB =
        results.freeSpaceLoss_dB +
        results.atmosphericLoss_Total_dB;

    return results;
}

AtmosphericModel::AtmosphericModel() {
}

double AtmosphericModel::calculateGaseousAttenuation(double frequency_MHz) {

if (frequency_MHz < 100.0) {
    return 0.001;
    } else if (frequency_MHz < 1000.0) {
        return 0.01;
    } else if (frequency_MHz < 10000.0) {
        double f_GHz = frequency_MHz / 1000.0;
        return 0.01 + (f_GHz - 1.0) * 0.01;
    } else if (frequency_MHz < 24000.0) {
        double f_GHz = frequency_MHz / 1000.0;
        return 0.1 + (f_GHz - 10.0) * 0.02;
    } else {
        double f_GHz = frequency_MHz / 1000.0;
        return 0.4 + (f_GHz - 24.0) * 0.05;
    }
}

double AtmosphericModel::getZenithAttenuation(double frequency_MHz) {
    return calculateGaseousAttenuation(frequency_MHz);
}

double AtmosphericModel::getSlantAttenuation(
    double frequency_MHz,
    double elevation_deg) {

    if (elevation_deg < 0) {
        return 0.0;
    }

    double zenithAtten_dB = getZenithAttenuation(frequency_MHz);

    double elevation_rad = elevation_deg * M_PI / 180.0;

    double sinEl = std::sin(elevation_rad);

    if (sinEl < 0.1) {
        double h0 = 8.0;
        double Re = 6371.0;
        double chi = M_PI / 2.0 - elevation_rad;
        double slantFactor = std::sqrt(
            (Re / h0) * (Re / h0) * std::cos(chi) * std::cos(chi) + 2.0 * (Re / h0) + 1.0
        ) - (Re / h0) * std::cos(chi);
        return zenithAtten_dB * slantFactor;
    } else {
        double slantFactor = 1.0 / sinEl;
        return zenithAtten_dB * slantFactor;
    }
}
