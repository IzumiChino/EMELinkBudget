#pragma once

#include "LinkBudgetTypes.h"
#include <cmath>

// ========== SNR Calculator ==========
class SNRCalculator {
public:
    SNRCalculator();

    SNRResults calculate(
        double txPower_dBm,
        double txGain_dBi,
        double rxGain_dBi,
        double txFeedlineLoss_dB,
        double rxFeedlineLoss_dB,
        const PathLossResults& pathLoss,
        const PolarizationResults& polarization,
        const NoiseResults& noise,
        double requiredSNR_dB = -30.2,
        double fadingMargin_dB = 3.0);

    double calculateReceivedPower(
        double txPower_dBm,
        double txGain_dBi,
        double rxGain_dBi,
        double txFeedlineLoss_dB,
        double rxFeedlineLoss_dB,
        double totalLoss_dB);

    double calculateSNR(
        double receivedPower_dBm,
        double noisePower_dBm);

    double calculateLinkMargin(
        double effectiveSNR_dB,
        double requiredSNR_dB);

private:
    double dBmToWatts(double power_dBm) const;
    double wattsTodBm(double power_W) const;
};

// ========== Fading Margin Analyzer ==========
class FadingMargin {
public:
    FadingMargin();

    double calculateMargin(
        double frequency_MHz,
        double pathLength_km);

    double getRecommendedMargin(
        double frequency_MHz,
        double reliability_percent = 90.0);

private:
    double estimateLibrationFading(double frequency_MHz);
};
