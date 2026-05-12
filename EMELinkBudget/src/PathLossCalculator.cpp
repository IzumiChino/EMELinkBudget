#include "PathLossCalculator.h"
#include "MathConstants.h"
#include <cmath>
#include <algorithm>

// ========== PathLossCalculator Implementation ==========

PathLossCalculator::PathLossCalculator() {
}

double PathLossCalculator::deg2rad(double degrees) const {
    return eme::math::degreesToRadians(degrees);
}

double PathLossCalculator::calculateFreeSpaceLoss(
    double frequency_MHz,
    double distance_km) {

    double frequency_Hz = frequency_MHz * 1e6;
    double distance_m = distance_km * 1000.0;

    double wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    double loss_dB = 20.0 * std::log10((4.0 * eme::math::kPi * distance_m) / wavelength_m);

    return loss_dB;
}

double PathLossCalculator::calculateLunarScatteringLoss(double reflectivity) {
    double geometricArea_m2 = eme::math::kPi * (MOON_RADIUS_KM * 1000.0) * (MOON_RADIUS_KM * 1000.0);
    double sigma_m2 = reflectivity * geometricArea_m2;

    double moonGain_dB = 10.0 * std::log10(sigma_m2);

    return -moonGain_dB;
}

// ========== Hagfors' Law Implementation ==========

double PathLossCalculator::calculateHagforsRoughnessParameter(double frequency_MHz) {
    // Hagfors (1964) roughness parameter C increases with frequency because
    // shorter wavelengths resolve finer surface structure -> appears rougher.
    // Values below roughly follow Evans & Hagfors (1968) for the near-side Moon.

    double C;
    if (frequency_MHz < 150.0) {
        C = 0.02;
    } else if (frequency_MHz < 500.0) {
        C = 0.03;
    } else if (frequency_MHz < 1500.0) {
        C = 0.05;
    } else if (frequency_MHz < 3000.0) {
        C = 0.08;
    } else {
        C = 0.12;
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
    double moonGeometricArea_m2 = eme::math::kPi * (MOON_RADIUS_KM * 1000.0) * (MOON_RADIUS_KM * 1000.0);
    double baseReflectivity = 0.07;  // Average lunar reflectivity

    // Total RCS with Hagfors correction
    double sigma_m2 = baseReflectivity * moonGeometricArea_m2 * scatteringFunction;

    return sigma_m2;
}

double PathLossCalculator::calculateBistaticAngle(
    double /*elevation_TX_deg*/,
    double /*elevation_RX_deg*/,
    double /*distance_TX_km*/,
    double /*distance_RX_km*/) {

    // True bistatic angle = angle at moon surface between TX and RX lines of sight.
    // Bounded by 2*R_earth / d_moon ~= 1.9 deg for any terrestrial pair.
    // At this magnitude cos^4(phi) + C*sin^2(phi) ~= 1 for all C in (0, 1),
    // so Hagfors correction is essentially unity; treat EME as monostatic.
    return 0.0;
}

double PathLossCalculator::calculateLunarScatteringLossHagfors(
    double frequency_MHz,
    double bistaticAngle_deg,
    double& rcs_dBsm,
    double& roughnessParam) {

    // Calculate frequency-dependent roughness parameter
    roughnessParam = calculateHagforsRoughnessParameter(frequency_MHz);

    // Convert bistatic angle to radians
    double bistaticAngle_rad = eme::math::degreesToRadians(bistaticAngle_deg);

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

    double elevation_rad = eme::math::degreesToRadians(elevation_deg);

    double sinEl = std::sin(elevation_rad);

    if (sinEl < 0.1) {
        double h0 = 8.0;
        double Re = 6371.0;
        double chi = eme::math::kPi / 2.0 - elevation_rad;
        double slantFactor = std::sqrt(
            (Re / h0) * (Re / h0) * std::cos(chi) * std::cos(chi) + 2.0 * (Re / h0) + 1.0
        ) - (Re / h0) * std::cos(chi);
        return zenithAtten_dB * slantFactor;
    } else {
        double slantFactor = 1.0 / sinEl;
        return zenithAtten_dB * slantFactor;
    }
}
