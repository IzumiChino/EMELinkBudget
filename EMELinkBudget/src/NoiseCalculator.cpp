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
    double rxGain_dBi,
    double physicalTemp_K) {

    if (elevation_deg < 0) {
        return physicalTemp_K;
    }

    double gainLinear = std::pow(10.0, rxGain_dBi / 10.0);
    double beamwidth_deg = 70.0 / std::sqrt(gainLinear);

    double criticalElevation = 1.5 * beamwidth_deg;

    if (elevation_deg > criticalElevation) {
        return physicalTemp_K * 0.02;
    }

    double spilloverFraction = 0.0;
    int numSamples = 36;
    double angleStep = beamwidth_deg * 3.0 / numSamples;

    for (int i = 0; i < numSamples; ++i) {
        double theta = i * angleStep;
        double patternValue = std::pow(std::cos(theta * M_PI / 180.0 / beamwidth_deg * 1.5), 2.0);

        if (patternValue < 0.01) patternValue = 0.01;

        double pointingAngle = elevation_deg - theta;

        if (pointingAngle < 0) {
            spilloverFraction += patternValue * angleStep;
        }
    }

    double normalizedSpillover = spilloverFraction / (beamwidth_deg * 3.0);

    return physicalTemp_K * normalizedSpillover;
}

double NoiseCalculator::calculateMoonBodyTemp() {
    return 1.0;
}

double NoiseCalculator::calculateAntennaEffectiveTemp(
    double antennaTemp_K,
    double feedlineLoss_dB,
    double physicalTemp_K) {

    double lossLinear = std::pow(10.0, feedlineLoss_dB / 10.0);

    double T_ant_attenuated = antennaTemp_K / lossLinear;
    double T_feedline_noise = physicalTemp_K * (1.0 - 1.0 / lossLinear);

    return T_ant_attenuated + T_feedline_noise;
}

double NoiseCalculator::calculateReceiverNoiseTemp(double noiseFigure_dB) {
    double noiseFactor = std::pow(10.0, noiseFigure_dB / 10.0);
    return 290.0 * (noiseFactor - 1.0);
}

double NoiseCalculator::calculateNoisePower(
    double systemTemp_K,
    double bandwidth_Hz) {

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

    results.skyNoiseTemp_K = calculateSkyNoiseTemp(
        frequency_MHz, moonRA_deg, moonDEC_deg);

    if (includeGroundSpillover) {
        results.groundSpilloverTemp_K = calculateGroundSpilloverTemp(
            elevation_deg, rxGain_dBi, physicalTemp_K);
    } else {
        results.groundSpilloverTemp_K = 0.0;
    }

    results.moonBodyTemp_K = calculateMoonBodyTemp();

    results.antennaNoiseTemp_K =
        results.skyNoiseTemp_K +
        results.groundSpilloverTemp_K +
        results.moonBodyTemp_K;

    results.antennaEffectiveTemp_K = calculateAntennaEffectiveTemp(
        results.antennaNoiseTemp_K,
        feedlineLoss_dB,
        physicalTemp_K);

    results.receiverNoiseTemp_K = calculateReceiverNoiseTemp(noiseFigure_dB);

    results.systemNoiseTemp_K =
        results.antennaEffectiveTemp_K +
        results.receiverNoiseTemp_K;

    results.noisePower_W = calculateNoisePower(
        results.systemNoiseTemp_K,
        bandwidth_Hz);

    results.noisePower_dBm = 10.0 * std::log10(results.noisePower_W * 1000.0);

    return results;
}

// ========== SkyNoiseModel Implementation ==========

SkyNoiseModel::SkyNoiseModel()
    : m_haslamMap(nullptr), m_skyMapLoaded(false) {
}

SkyNoiseModel::~SkyNoiseModel() {
}

double SkyNoiseModel::estimateGalacticLatitude(double ra_deg, double dec_deg) {

double galacticLat_approx = std::abs(dec_deg);

if (ra_deg > 240.0 && ra_deg < 300.0) {
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

    if (m_skyMapLoaded && m_haslamMap && m_haslamMap->isLoaded()) {
        double T_408 = m_haslamMap->getTemperature(ra_deg, dec_deg);
        if (T_408 > 0.0) {
            return T_408 * std::pow(frequency_MHz / 408.0, SPECTRAL_INDEX);
        }
    }

    double galacticLat = estimateGalacticLatitude(ra_deg, dec_deg);
    return calculateSkyTemp_Simplified(frequency_MHz, galacticLat);
}

bool SkyNoiseModel::loadSkyMap(const std::string& mapPath) {
    m_haslamMap = std::make_unique<HaslamSkyMap>();
    m_skyMapLoaded = m_haslamMap->loadFITS(mapPath);
    return m_skyMapLoaded;
}

bool SkyNoiseModel::isMapLoaded() const {
    return m_skyMapLoaded && m_haslamMap && m_haslamMap->isLoaded();
}
