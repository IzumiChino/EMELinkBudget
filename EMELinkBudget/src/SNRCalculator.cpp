#include "SNRCalculator.h"
#include <cmath>
#include <algorithm>

// ========== SNRCalculator Implementation ==========

SNRCalculator::SNRCalculator() {
}

double SNRCalculator::dBmToWatts(double power_dBm) const {
    return std::pow(10.0, (power_dBm - 30.0) / 10.0);
}

double SNRCalculator::wattsTodBm(double power_W) const {
    return 10.0 * std::log10(power_W) + 30.0;
}

double SNRCalculator::calculateReceivedPower(
    double txPower_dBm,
    double txGain_dBi,
    double rxGain_dBi,
    double txFeedlineLoss_dB,
    double rxFeedlineLoss_dB,
    double totalLoss_dB) {

    double receivedPower_dBm =
        txPower_dBm +
        txGain_dBi +
        rxGain_dBi -
        txFeedlineLoss_dB -
        rxFeedlineLoss_dB -
        totalLoss_dB;

    return receivedPower_dBm;
}

double SNRCalculator::calculateSNR(
    double receivedPower_dBm,
    double noisePower_dBm) {

    return receivedPower_dBm - noisePower_dBm;
}

double SNRCalculator::calculateLinkMargin(
    double effectiveSNR_dB,
    double requiredSNR_dB) {

    return effectiveSNR_dB - requiredSNR_dB;
}

SNRResults SNRCalculator::calculate(
    double txPower_dBm,
    double txGain_dBi,
    double rxGain_dBi,
    double txFeedlineLoss_dB,
    double rxFeedlineLoss_dB,
    const PathLossResults& pathLoss,
    const PolarizationResults& polarization,
    const NoiseResults& noise,
    double requiredSNR_dB,
    double fadingMargin_dB) {

    SNRResults results;

    double totalLoss_dB =
        pathLoss.totalPathLoss_dB +
        polarization.polarizationLoss_dB;

    results.receivedSignalPower_dBm = calculateReceivedPower(
        txPower_dBm,
        txGain_dBi,
        rxGain_dBi,
        txFeedlineLoss_dB,
        rxFeedlineLoss_dB,
        totalLoss_dB);

    results.receivedSignalPower_W = dBmToWatts(results.receivedSignalPower_dBm);

    results.SNR_dB = calculateSNR(
        results.receivedSignalPower_dBm,
        noise.noisePower_dBm);

    results.fadingMargin_dB = fadingMargin_dB;
    results.effectiveSNR_dB = results.SNR_dB - results.fadingMargin_dB;

    results.requiredSNR_dB = requiredSNR_dB;
    results.linkMargin_dB = calculateLinkMargin(
        results.effectiveSNR_dB,
        results.requiredSNR_dB);

    results.linkViable = (results.linkMargin_dB > 0.0);

    return results;
}

// ========== FadingMargin Implementation ==========

FadingMargin::FadingMargin() {
}

double FadingMargin::estimateLibrationFading(double frequency_MHz) {
    if (frequency_MHz < 200.0) {
        return 2.5;
    } else if (frequency_MHz < 500.0) {
        return 3.0;
    } else if (frequency_MHz < 1500.0) {
        return 3.5;
    } else if (frequency_MHz < 5000.0) {
        return 4.5;
    } else {
        return 5.5;
    }
}

double FadingMargin::calculateMargin(
    double frequency_MHz,
    double pathLength_km) {

    double baseMargin = estimateLibrationFading(frequency_MHz);

    double pathMargin = 0.5;

    return baseMargin + pathMargin;
}

double FadingMargin::getRecommendedMargin(
    double frequency_MHz,
    double reliability_percent) {

    double baseMargin = calculateMargin(frequency_MHz, 384400.0);

    if (reliability_percent >= 99.0) {
        return baseMargin + 2.0;
    } else if (reliability_percent >= 95.0) {
        return baseMargin + 1.0;
    } else if (reliability_percent >= 90.0) {
        return baseMargin;
    } else {
        return baseMargin - 1.0;
    }
}
