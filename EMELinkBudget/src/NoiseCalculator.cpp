#include "NoiseCalculator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== NoiseCalculator Implementation ==========

NoiseCalculator::NoiseCalculator() {
}

double NoiseCalculator::calculateSkyNoiseTemp(
    double frequency_MHz,
    double moonRA_deg,
    double moonDEC_deg) {

    SkyNoiseModel skyModel;
    return skyModel.getSkyTemp(frequency_MHz, moonRA_deg, moonDEC_deg);
}

double NoiseCalculator::calculateGroundSpilloverTemp(
    double elevation_deg,
    double physicalTemp_K) {

    if (elevation_deg < 0) {
        return physicalTemp_K;  // Moon below horizon
    }

    // Ground spillover model
    // At high elevations, minimal spillover
    // At low elevations, significant spillover

    double spilloverFactor = 0.0;

    if (elevation_deg < 10.0) {
        // Very low elevation: high spillover
        spilloverFactor = 0.3 - (elevation_deg / 10.0) * 0.2;
    } else if (elevation_deg < 30.0) {
        // Low to medium elevation: moderate spillover
        spilloverFactor = 0.1 - ((elevation_deg - 10.0) / 20.0) * 0.08;
    } else {
        // High elevation: minimal spillover
        spilloverFactor = 0.02;
    }

    return physicalTemp_K * spilloverFactor;
}

double NoiseCalculator::calculateMoonBodyTemp() {
    // Moon body temperature contribution
    // The moon is a thermal source at ~250K
    // But its solid angle is very small compared to antenna beamwidth
    // For typical EME antennas, this is negligible (<1K)
    return 1.0;
}

double NoiseCalculator::calculateAntennaEffectiveTemp(
    double antennaTemp_K,
    double feedlineLoss_dB,
    double physicalTemp_K) {

    // Convert loss from dB to linear
    double lossLinear = std::pow(10.0, feedlineLoss_dB / 10.0);

    // Antenna temperature seen at receiver input
    // T_ant_eff = T_ant / L + T_phy * (1 - 1/L)
    double T_ant_attenuated = antennaTemp_K / lossLinear;
    double T_feedline_noise = physicalTemp_K * (1.0 - 1.0 / lossLinear);

    return T_ant_attenuated + T_feedline_noise;
}

double NoiseCalculator::calculateReceiverNoiseTemp(double noiseFigure_dB) {
    // Convert noise figure to noise temperature
    // T_rx = T_0 * (10^(NF/10) - 1)
    // where T_0 = 290 K (standard reference temperature)

    double noiseFactor = std::pow(10.0, noiseFigure_dB / 10.0);
    return 290.0 * (noiseFactor - 1.0);
}

double NoiseCalculator::calculateNoisePower(
    double systemTemp_K,
    double bandwidth_Hz) {

    // Noise power: P_N = k_B * T_sys * B
    double noisePower_W = BOLTZMANN_CONSTANT * systemTemp_K * bandwidth_Hz;

    return noisePower_W;
}

NoiseResults NoiseCalculator::calculate(
    double frequency_MHz,
    double bandwidth_Hz,
    double rxGain_dBi,
    double feedlineLoss_dB,
    double noiseFigure_dB,
    double elevation_deg,
    double moonRA_deg,
    double moonDEC_deg,
    double physicalTemp_K,
    bool includeGroundSpillover) {

    NoiseResults results;

    // Calculate sky noise temperature
    results.skyNoiseTemp_K = calculateSkyNoiseTemp(
        frequency_MHz, moonRA_deg, moonDEC_deg);

    // Calculate ground spillover
    if (includeGroundSpillover) {
        results.groundSpilloverTemp_K = calculateGroundSpilloverTemp(
            elevation_deg, physicalTemp_K);
    } else {
        results.groundSpilloverTemp_K = 0.0;
    }

    // Calculate moon body temperature
    results.moonBodyTemp_K = calculateMoonBodyTemp();

    // Total antenna noise temperature
    results.antennaNoiseTemp_K =
        results.skyNoiseTemp_K +
        results.groundSpilloverTemp_K +
        results.moonBodyTemp_K;

    // Antenna effective temperature (after feedline)
    results.antennaEffectiveTemp_K = calculateAntennaEffectiveTemp(
        results.antennaNoiseTemp_K,
        feedlineLoss_dB,
        physicalTemp_K);

    // Receiver noise temperature
    results.receiverNoiseTemp_K = calculateReceiverNoiseTemp(noiseFigure_dB);

    // System noise temperature
    results.systemNoiseTemp_K =
        results.antennaEffectiveTemp_K +
        results.receiverNoiseTemp_K;

    // Noise power
    results.noisePower_W = calculateNoisePower(
        results.systemNoiseTemp_K,
        bandwidth_Hz);

    // Convert to dBm
    results.noisePower_dBm = 10.0 * std::log10(results.noisePower_W * 1000.0);

    return results;
}

// ========== SkyNoiseModel Implementation ==========

SkyNoiseModel::SkyNoiseModel()
    : m_skyMapLoaded(false) {
}

double SkyNoiseModel::estimateGalacticLatitude(double ra_deg, double dec_deg) {
// Simplified conversion from equatorial to galactic coordinates
// Galactic center is approximately at RA=266 deg, DEC=-29 deg
// Galactic north pole is approximately at RA=192.85 deg, DEC=27.13 deg

// For simplicity, use declination as rough proxy
// High |DEC| -> high galactic latitude (cold sky)
// Low |DEC| -> low galactic latitude (warm sky, near galactic plane)

    double galacticLat_approx = std::abs(dec_deg);

    // Adjust based on RA (galactic plane crosses equator)
    if (ra_deg > 240.0 && ra_deg < 300.0) {
        // Near galactic center direction
        galacticLat_approx *= 0.5;
    }

    return galacticLat_approx;
}

double SkyNoiseModel::calculateSkyTemp_Simplified(
    double frequency_MHz,
    double galacticLatitude_deg) {

    // Get 408 MHz temperature based on galactic latitude
    double T_408;

    if (galacticLatitude_deg > 60.0) {
        // High galactic latitude: cold sky
        T_408 = T_SKY_408_COLD;
    } else if (galacticLatitude_deg < 20.0) {
        // Low galactic latitude: warm sky (galactic plane)
        T_408 = T_SKY_408_WARM;
    } else {
        // Intermediate: linear interpolation
        double factor = (galacticLatitude_deg - 20.0) / 40.0;
        T_408 = T_SKY_408_WARM + factor * (T_SKY_408_COLD - T_SKY_408_WARM);
    }

    // Scale to target frequency using spectral index
    // T(f) = T_408 * (f/408)^alpha
    // where alpha ~= -2.55 for galactic synchrotron radiation
    double T_sky = T_408 * std::pow(frequency_MHz / 408.0, SPECTRAL_INDEX);

    return T_sky;
}

double SkyNoiseModel::getSkyTemp(
    double frequency_MHz,
    double ra_deg,
    double dec_deg) {

    if (m_skyMapLoaded) {
        // Use loaded sky map (not implemented yet)
        // TODO: Implement 408 MHz map lookup
    }

    // Use simplified model
    double galacticLat = estimateGalacticLatitude(ra_deg, dec_deg);
    return calculateSkyTemp_Simplified(frequency_MHz, galacticLat);
}

bool SkyNoiseModel::loadSkyMap(const std::string& mapPath) {
    // TODO: Implement 408 MHz sky map loading
    // For now, return false (not implemented)
    m_skyMapLoaded = false;
    return false;
}
