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

    // Link budget equation:
    // P_RX = P_TX + G_TX + G_RX - L_feedline_TX - L_feedline_RX - L_total

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

    // SNR = P_signal - P_noise (in dB)
    return receivedPower_dBm - noisePower_dBm;
}

double SNRCalculator::calculateLinkMargin(
    double effectiveSNR_dB,
    double requiredSNR_dB) {

    // Link margin = Effective SNR - Required SNR
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

    // Total loss = Path loss + Polarization loss
    double totalLoss_dB =
        pathLoss.totalPathLoss_dB +
        polarization.polarizationLoss_dB;

    // Calculate received signal power
    results.receivedSignalPower_dBm = calculateReceivedPower(
        txPower_dBm,
        txGain_dBi,
        rxGain_dBi,
        txFeedlineLoss_dB,
        rxFeedlineLoss_dB,
        totalLoss_dB);

    // Convert to watts
    results.receivedSignalPower_W = dBmToWatts(results.receivedSignalPower_dBm);

    // Calculate SNR
    results.SNR_dB = calculateSNR(
        results.receivedSignalPower_dBm,
        noise.noisePower_dBm);

    // Apply fading margin
    results.fadingMargin_dB = fadingMargin_dB;
    results.effectiveSNR_dB = results.SNR_dB - results.fadingMargin_dB;

    // Calculate link margin
    results.requiredSNR_dB = requiredSNR_dB;
    results.linkMargin_dB = calculateLinkMargin(
        results.effectiveSNR_dB,
        results.requiredSNR_dB);

    // Determine if link is viable
    results.linkViable = (results.linkMargin_dB > 0.0);

    return results;
}

// ========== FadingMargin Implementation ==========

FadingMargin::FadingMargin() {
}

double FadingMargin::estimateLibrationFading(double frequency_MHz) {
    // Libration fading characteristics depend on frequency
    // Lower frequencies: more specular reflection, less fading
    // Higher frequencies: more diffuse scattering, more fading

    if (frequency_MHz < 200.0) {
        // VHF: primarily specular reflection
        // Shallow fading, typically 2-3 dB
        return 2.5;
    } else if (frequency_MHz < 500.0) {
        // Low UHF: mixed specular and diffuse
        return 3.0;
    } else if (frequency_MHz < 1500.0) {
        // UHF: increasing diffuse component
        return 3.5;
    } else if (frequency_MHz < 5000.0) {
        // High UHF / Low SHF: predominantly diffuse
        // Rayleigh-like fading
        return 4.5;
    } else {
        // SHF and above: strong diffuse scattering
        return 5.5;
    }
}

double FadingMargin::calculateMargin(
    double frequency_MHz,
    double pathLength_km) {

    // Base margin from libration fading
    double baseMargin = estimateLibrationFading(frequency_MHz);

    // Additional margin for path length variations
    // (minor effect, ~0.5 dB for typical moon distance variations)
    double pathMargin = 0.5;

    return baseMargin + pathMargin;
}

double FadingMargin::getRecommendedMargin(
    double frequency_MHz,
    double reliability_percent) {

    // Base margin for 90% reliability
    double baseMargin = calculateMargin(frequency_MHz, 384400.0);

    // Adjust for different reliability requirements
    if (reliability_percent >= 99.0) {
        // 99% reliability: add 2 dB
        return baseMargin + 2.0;
    } else if (reliability_percent >= 95.0) {
        // 95% reliability: add 1 dB
        return baseMargin + 1.0;
    } else if (reliability_percent >= 90.0) {
        // 90% reliability: base margin
        return baseMargin;
    } else {
        // Lower reliability: reduce margin
        return baseMargin - 1.0;
    }
}
