#pragma once

#include "LinkBudgetTypes.h"
#include "HaslamSkyMap.h"
#include <cmath>
#include <string>
#include <memory>

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

class SkyNoiseModel {
public:
    SkyNoiseModel();
    ~SkyNoiseModel();

    double getSkyTemp(
        double frequency_MHz,
        double ra_deg,
        double dec_deg);

    bool loadSkyMap(const std::string& mapPath);
    bool isMapLoaded() const;

private:
    double calculateSkyTemp_Simplified(
        double frequency_MHz,
        double galacticLatitude_deg);

    double estimateGalacticLatitude(double ra_deg, double dec_deg);

    std::unique_ptr<HaslamSkyMap> m_haslamMap;
    bool m_skyMapLoaded;
    static constexpr double T_SKY_408_COLD = 20.0;
    static constexpr double T_SKY_408_WARM = 150.0;
    static constexpr double SPECTRAL_INDEX = -2.55;
};
