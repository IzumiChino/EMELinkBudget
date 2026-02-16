#pragma once

#include "LinkBudgetTypes.h"
#include <cmath>
#include <string>

// ========== Noise Calculator ==========
class NoiseCalculator {
public:
    NoiseCalculator();

    NoiseResults calculate(
        double frequency_MHz,
        double bandwidth_Hz,
        double rxGain_dBi,
        double feedlineLoss_dB,
        double noiseFigure_dB,
        double elevation_deg,
        double moonRA_deg,
        double moonDEC_deg,
        double physicalTemp_K = 290.0,
        bool includeGroundSpillover = true);

    double calculateSkyNoiseTemp(
        double frequency_MHz,
        double moonRA_deg,
        double moonDEC_deg);

    double calculateGroundSpilloverTemp(
        double elevation_deg,
        double physicalTemp_K = 290.0);

    double calculateMoonBodyTemp();

    double calculateAntennaEffectiveTemp(
        double antennaTemp_K,
        double feedlineLoss_dB,
        double physicalTemp_K = 290.0);

    double calculateReceiverNoiseTemp(
        double noiseFigure_dB);

    double calculateNoisePower(
        double systemTemp_K,
        double bandwidth_Hz);

private:
static constexpr double BOLTZMANN_CONSTANT = 1.38064852e-23;
};

// ========== Sky Noise Model ==========
class SkyNoiseModel {
public:
    SkyNoiseModel();

    double getSkyTemp(
        double frequency_MHz,
        double ra_deg,
        double dec_deg);

    bool loadSkyMap(const std::string& mapPath);

private:
    double calculateSkyTemp_Simplified(
        double frequency_MHz,
        double galacticLatitude_deg);

    // Convert RA/DEC to Galactic coordinates (simplified)
    double estimateGalacticLatitude(double ra_deg, double dec_deg);

    bool m_skyMapLoaded;
    // 408 MHz sky temperature reference values (from Haslam map)
    static constexpr double T_SKY_408_COLD = 20.0;   // K at 408 MHz, cold sky (high galactic latitude)
    static constexpr double T_SKY_408_WARM = 150.0;  // K at 408 MHz, galactic plane
    static constexpr double SPECTRAL_INDEX = -2.55;  // Spectral index for synchrotron radiation
};
