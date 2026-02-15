#pragma once

#include "LinkBudgetTypes.h"
#include <cmath>

// ========== SNR Calculator ==========
class SNRCalculator {
public:
    SNRCalculator();

    // Calculate SNR and link margin
    SNRResults calculate(
        double txPower_dBm,
        double txGain_dBi,
        double rxGain_dBi,
        double txFeedlineLoss_dB,
        double rxFeedlineLoss_dB,
        const PathLossResults& pathLoss,
        const PolarizationResults& polarization,
        const NoiseResults& noise,
        double requiredSNR_dB = -21.0,
        double fadingMargin_dB = 3.0);

    // Calculate received signal power
    double calculateReceivedPower(
        double txPower_dBm,
        double txGain_dBi,
        double rxGain_dBi,
        double txFeedlineLoss_dB,
        double rxFeedlineLoss_dB,
        double totalLoss_dB);

    // Calculate SNR
    double calculateSNR(
        double receivedPower_dBm,
        double noisePower_dBm);

    // Calculate link margin
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

    // Calculate fading margin based on frequency and link characteristics
    double calculateMargin(
        double frequency_MHz,
        double pathLength_km);

    // Get recommended margin for given reliability
    double getRecommendedMargin(
        double frequency_MHz,
        double reliability_percent = 90.0);

private:
    // Libration fading characteristics
    double estimateLibrationFading(double frequency_MHz);
};
