#include "EMELinkBudget.h"
#include <sstream>
#include <ctime>

// ========== EMELinkBudget Implementation ==========

EMELinkBudget::EMELinkBudget() {
}

EMELinkBudget::EMELinkBudget(const LinkBudgetParameters& params)
    : m_params(params) {
}

void EMELinkBudget::setParameters(const LinkBudgetParameters& params) {
    m_params = params;
}

bool EMELinkBudget::validateParameters(std::string& errorMsg) const {
    std::ostringstream oss;

    if (m_params.frequency_MHz <= 0) {
        oss << "Invalid frequency: " << m_params.frequency_MHz << " MHz. ";
    }

    if (m_params.bandwidth_Hz <= 0) {
        oss << "Invalid bandwidth: " << m_params.bandwidth_Hz << " Hz. ";
    }

    if (m_params.txPower_dBm < -50.0 || m_params.txPower_dBm > 100.0) {
        oss << "TX power out of reasonable range: " << m_params.txPower_dBm << " dBm. ";
    }

    if (m_params.txGain_dBi < 0.0 || m_params.txGain_dBi > 50.0) {
        oss << "TX gain out of reasonable range: " << m_params.txGain_dBi << " dBi. ";
    }
    if (m_params.rxGain_dBi < 0.0 || m_params.rxGain_dBi > 50.0) {
        oss << "RX gain out of reasonable range: " << m_params.rxGain_dBi << " dBi. ";
    }

    if (m_params.rxNoiseFigure_dB < 0.0 || m_params.rxNoiseFigure_dB > 10.0) {
        oss << "RX noise figure out of reasonable range: " << m_params.rxNoiseFigure_dB << " dB. ";
    }

    errorMsg = oss.str();
    return errorMsg.empty();
}

GeometryResults EMELinkBudget::calculateGeometry() {
    return m_geometryCalc.calculate(
        m_params.txSite,
        m_params.rxSite,
        m_params.moonEphemeris,
        m_params.observationTime);
}

PathLossResults EMELinkBudget::calculatePathLoss(const GeometryResults& geometry) {
    return m_pathLossCalc.calculate(
        m_params.frequency_MHz,
        geometry.distance_TX_km,
        geometry.distance_RX_km,
        geometry.moonElevation_TX_deg,
        geometry.moonElevation_RX_deg,
        m_params.includeAtmosphericLoss,
        m_params.useHagforsModel);
}

PolarizationResults EMELinkBudget::calculatePolarization(const GeometryResults& geometry) {
    return m_polarizationModule.calculate(m_params, geometry);
}

NoiseResults EMELinkBudget::calculateNoise(const GeometryResults& geometry) {
    return m_noiseCalc.calculate(
        m_params.frequency_MHz,
        m_params.bandwidth_Hz,
        m_params.rxGain_dBi,
        m_params.rxFeedlineLoss_dB,
        m_params.rxNoiseFigure_dB,
        geometry.moonElevation_RX_deg,
        geometry.moonRA_deg,
        geometry.moonDEC_deg,
        m_params.physicalTemp_K,
        m_params.includeGroundSpillover);
}

SNRResults EMELinkBudget::calculateSNR(
    const PathLossResults& pathLoss,
    const PolarizationResults& polarization,
    const NoiseResults& noise) {

    FadingMargin fadingAnalyzer;
    double fadingMargin = fadingAnalyzer.calculateMargin(
        m_params.frequency_MHz,
        pathLoss.totalPathLoss_dB);

    return m_snrCalc.calculate(
        m_params.txPower_dBm,
        m_params.txGain_dBi,
        m_params.rxGain_dBi,
        m_params.txFeedlineLoss_dB,
        m_params.rxFeedlineLoss_dB,
        pathLoss,
        polarization,
        noise,
        -30.2,
        fadingMargin);
}

LinkBudgetResults EMELinkBudget::calculate() {
    m_lastResults = LinkBudgetResults();
    m_lastResults.calculationTime = std::time(nullptr);

    try {
        std::string errorMsg;
        if (!validateParameters(errorMsg)) {
            m_lastResults.calculationSuccess = false;
            m_lastResults.errorMessage = errorMsg;
            return m_lastResults;
        }

        m_lastResults.geometry = calculateGeometry();

        m_lastResults.pathLoss = calculatePathLoss(m_lastResults.geometry);

        m_lastResults.polarization = calculatePolarization(m_lastResults.geometry);

        m_lastResults.noise = calculateNoise(m_lastResults.geometry);

        m_lastResults.snr = calculateSNR(
            m_lastResults.pathLoss,
            m_lastResults.polarization,
            m_lastResults.noise);

        m_lastResults.totalLoss_dB =
            m_lastResults.pathLoss.totalPathLoss_dB +
            m_lastResults.polarization.polarizationLoss_dB;

        m_lastResults.calculationSuccess = true;

    } catch (const std::exception& e) {
        m_lastResults.calculationSuccess = false;
        m_lastResults.errorMessage = std::string("Calculation error: ") + e.what();
    }

    return m_lastResults;
}
