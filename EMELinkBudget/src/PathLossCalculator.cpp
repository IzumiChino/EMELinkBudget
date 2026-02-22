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

// ========== Hagfors' Law Implementation ==========

double PathLossCalculator::calculateHagforsRoughnessParameter(double frequency_MHz) {
    // Surface roughness parameter C depends on frequency and surface characteristics
    // Empirical model based on lunar radar observations
    // Reference: Hagfors (1964), Evans & Hagfors (1968)

    // For the Moon, typical values:
    // - Low frequencies (50-150 MHz): C ≈ 0.1 - 0.2 (smoother appearance)
    // - Mid frequencies (400-1000 MHz): C ≈ 0.05 - 0.1
    // - High frequencies (>2 GHz): C ≈ 0.02 - 0.05 (rougher appearance)

    double C;
    if (frequency_MHz < 150.0) {
        C = 0.15;
    } else if (frequency_MHz < 500.0) {
        C = 0.10;
    } else if (frequency_MHz < 1500.0) {
        C = 0.07;
    } else if (frequency_MHz < 3000.0) {
        C = 0.05;
    } else {
        C = 0.03;
    }

    return C;
}

double PathLossCalculator::calculateHagforsScatteringCrossSection(
    double bistaticAngle_rad,
    double roughnessParam) {

    // Hagfors' Law: σ(φ) = σ₀ · [cos⁴φ + C·sin²φ]^(-3/2)
    // where φ is the bistatic angle (incidence angle)

    double cos_phi = std::cos(bistaticAngle_rad);
    double sin_phi = std::sin(bistaticAngle_rad);

    // Avoid division by zero at grazing angles
    if (std::abs(cos_phi) < 0.01) {
        cos_phi = 0.01;
    }

    double cos4 = cos_phi * cos_phi * cos_phi * cos_phi;
    double sin2 = sin_phi * sin_phi;

    // Hagfors denominator
    double denominator = cos4 + roughnessParam * sin2;

    // Avoid numerical issues
    if (denominator < 1e-10) {
        denominator = 1e-10;
    }

    // Scattering function (normalized)
    double scatteringFunction = std::pow(denominator, -1.5);

    // Base radar cross-section (geometric area with average reflectivity)
    double moonGeometricArea_m2 = M_PI * (MOON_RADIUS_KM * 1000.0) * (MOON_RADIUS_KM * 1000.0);
    double baseReflectivity = 0.07;  // Average lunar reflectivity

    // Total RCS with Hagfors correction
    double sigma_m2 = baseReflectivity * moonGeometricArea_m2 * scatteringFunction;

    return sigma_m2;
}

double PathLossCalculator::calculateBistaticAngle(
    double elevation_TX_deg,
    double elevation_RX_deg,
    double distance_TX_km,
    double distance_RX_km) {

    // For EME, the bistatic angle is approximately related to the difference
    // in elevation angles from TX and RX stations
    //
    // Simplified model: bistatic angle ≈ |elevation_TX - elevation_RX| / 2
    // This is valid for small angular separations

    double elevDiff_deg = std::abs(elevation_TX_deg - elevation_RX_deg);

    // For more accurate calculation, consider the geometry:
    // The bistatic angle is the angle at the moon surface between
    // the incident and reflected rays

    // Average elevation (approximation for specular point)
    double avgElevation_deg = (elevation_TX_deg + elevation_RX_deg) / 2.0;

    // Bistatic angle (simplified model)
    // For near-specular reflection (typical EME), this is small
    double bistaticAngle_deg = elevDiff_deg / 2.0;

    // Clamp to reasonable range [0, 90 degrees]
    if (bistaticAngle_deg < 0.0) bistaticAngle_deg = 0.0;
    if (bistaticAngle_deg > 90.0) bistaticAngle_deg = 90.0;

    return bistaticAngle_deg;
}

double PathLossCalculator::calculateLunarScatteringLossHagfors(
    double frequency_MHz,
    double bistaticAngle_deg,
    double& rcs_dBsm,
    double& roughnessParam) {

    // Calculate frequency-dependent roughness parameter
    roughnessParam = calculateHagforsRoughnessParameter(frequency_MHz);

    // Convert bistatic angle to radians
    double bistaticAngle_rad = bistaticAngle_deg * M_PI / 180.0;

    // Calculate radar cross-section using Hagfors model
    double sigma_m2 = calculateHagforsScatteringCrossSection(
        bistaticAngle_rad, roughnessParam);

    // Convert to dBsm (dB relative to square meter)
    rcs_dBsm = 10.0 * std::log10(sigma_m2);

    // Moon gain in dB
    double moonGain_dB = rcs_dBsm;

    // Scattering loss is negative of gain
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
    bool includeAtmospheric,
    bool useHagforsModel) {

    PathLossResults results;

    double frequency_Hz = frequency_MHz * 1e6;
    results.wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    double distance_km = (distance_TX_km + distance_RX_km) / 2.0;

    double L_echo = 20.0 * std::log10(frequency_MHz) +
                    40.0 * std::log10(distance_km) -
                    14.6;

    results.freeSpaceLoss_dB = L_echo;

    // Lunar scattering loss - choose model
    results.useHagforsModel = useHagforsModel;

    if (useHagforsModel) {
        // Calculate bistatic angle
        results.bistaticAngle_deg = calculateBistaticAngle(
            elevation_TX_deg, elevation_RX_deg,
            distance_TX_km, distance_RX_km);

        // Use Hagfors' Law
        results.lunarScatteringLoss_dB = calculateLunarScatteringLossHagfors(
            frequency_MHz,
            results.bistaticAngle_deg,
            results.lunarRCS_dBsm,
            results.hagforsRoughnessParam);

        results.hagforsGain_dB = -results.lunarScatteringLoss_dB;
    } else {
        // Use simple reflectivity model
        results.lunarReflectivity = 0.07;
        results.lunarScatteringLoss_dB = calculateLunarScatteringLoss(results.lunarReflectivity);
        results.bistaticAngle_deg = 0.0;
        results.hagforsRoughnessParam = 0.0;
        results.lunarRCS_dBsm = 0.0;
        results.hagforsGain_dB = 0.0;
    }

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
