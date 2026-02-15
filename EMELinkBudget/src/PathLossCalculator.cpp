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

    // Convert frequency to Hz and distance to meters
    double frequency_Hz = frequency_MHz * 1e6;
    double distance_m = distance_km * 1000.0;

    // Calculate wavelength
    double wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    // Free space path loss: L_FS = 20*log10(4*pi*R/lambda)
    double loss_dB = 20.0 * std::log10((4.0 * M_PI * distance_m) / wavelength_m);

    return loss_dB;
}

double PathLossCalculator::calculateLunarScatteringLoss(double reflectivity) {
// Moon effective radar cross section (RCS)
// sigma_moon = rho * pi * R_moon^2
// where rho ~= 0.07 for VHF/UHF

double geometricArea_m2 = M_PI * (MOON_RADIUS_KM * 1000.0) * (MOON_RADIUS_KM * 1000.0);
    double sigma_m2 = reflectivity * geometricArea_m2;

    // Convert RCS to dB gain (this is the moon's "antenna gain")
    // G_moon = 10*log10(sigma)
    double moonGain_dB = 10.0 * std::log10(sigma_m2);

    // The moon gain REDUCES the path loss (it's a negative loss, i.e., a gain)
    // So we return a negative value
    return -moonGain_dB;
}

double PathLossCalculator::calculateAtmosphericLoss(
    double frequency_MHz,
    double elevation_deg) {

    if (elevation_deg < 0) {
        return 0.0;  // Moon below horizon
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

    // Calculate wavelength
    double frequency_Hz = frequency_MHz * 1e6;
    results.wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    // EME Radar Equation Path Loss
    // Corrected formula: L_echo = 20*log10(f_MHz) + 40*log10(d_km) - 14.6
    // The constant -14.6 accounts for:
    //   - Moon RCS: sigma = 0.07 * pi * (1738 km)^2
    //   - Radar equation geometry factor: (4*pi)^3
    //   - Unit conversions (MHz to Hz, km to m)
    // This prevents double-counting antenna aperture effects (lambda^2 factor)

    double distance_km = (distance_TX_km + distance_RX_km) / 2.0;  // Average distance

    // Calculate using corrected EME formula
    double L_echo = 20.0 * std::log10(frequency_MHz) +
                    40.0 * std::log10(distance_km) -
                    14.6;

    results.freeSpaceLoss_dB = L_echo;

    // Lunar scattering is already included in the standard formula
    // But we still calculate it separately for display purposes
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

// ========== AtmosphericModel Implementation ==========

AtmosphericModel::AtmosphericModel() {
}

double AtmosphericModel::calculateGaseousAttenuation(double frequency_MHz) {
    // Simplified atmospheric attenuation model
    // Based on ITU-R P.676 for clear air

    if (frequency_MHz < 100.0) {
        // Below 100 MHz: negligible attenuation
        return 0.001;
    } else if (frequency_MHz < 1000.0) {
        // 100-1000 MHz: very low attenuation
        return 0.01;
    } else if (frequency_MHz < 10000.0) {
        // 1-10 GHz: low to moderate attenuation
        // Linear interpolation
        double f_GHz = frequency_MHz / 1000.0;
        return 0.01 + (f_GHz - 1.0) * 0.01;
    } else if (frequency_MHz < 24000.0) {
        // 10-24 GHz: moderate attenuation
        double f_GHz = frequency_MHz / 1000.0;
        return 0.1 + (f_GHz - 10.0) * 0.02;
    } else {
        // Above 24 GHz: high attenuation
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

    // Get zenith attenuation
    double zenithAtten_dB = getZenithAttenuation(frequency_MHz);

    // Convert elevation to radians
    double elevation_rad = elevation_deg * M_PI / 180.0;

    // Calculate slant factor: 1/sin(elevation)
    // With correction for low elevations
    double sinEl = std::sin(elevation_rad);

    // Avoid division by very small numbers
    if (sinEl < 0.1) {
        // Use more accurate model for low elevations
        // Approximate with Chapman function
        double h0 = 8.0;  // Scale height in km
        double Re = 6371.0;  // Earth radius in km
        double chi = M_PI / 2.0 - elevation_rad;  // Zenith angle
        double slantFactor = std::sqrt(
            (Re / h0) * (Re / h0) * std::cos(chi) * std::cos(chi) + 2.0 * (Re / h0) + 1.0
        ) - (Re / h0) * std::cos(chi);
        return zenithAtten_dB * slantFactor;
    } else {
        // Simple 1/sin(El) model for higher elevations
        double slantFactor = 1.0 / sinEl;
        return zenithAtten_dB * slantFactor;
    }
}
