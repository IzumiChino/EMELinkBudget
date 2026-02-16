#include "PolarizationModule.h"

// ========== PolarizationModule Implementation ==========

PolarizationModule::PolarizationModule() {
}

void PolarizationModule::setupCalculator(
    const LinkBudgetParameters& params,
    const GeometryResults& geometry) {

    SystemConfiguration config;
    config.frequency_MHz = params.frequency_MHz;
    config.bandwidth_Hz = params.bandwidth_Hz;
    config.includeFaradayRotation = params.includeFaradayRotation;
    config.includeSpatialRotation = params.includeSpatialRotation;
    config.includeMoonReflection = params.includeMoonReflection;

    m_faradayCalc.setConfiguration(config);

    m_faradayCalc.setDXStation(params.txSite);
    m_faradayCalc.setHomeStation(params.rxSite);

    m_faradayCalc.setIonosphereData(params.ionosphereData);

    MoonEphemeris moonEphem = params.moonEphemeris;

    moonEphem.rightAscension = ParameterUtils::deg2rad(geometry.moonRA_deg);
    moonEphem.declination = ParameterUtils::deg2rad(geometry.moonDEC_deg);
    moonEphem.distance_km = geometry.moonDistance_km;
    moonEphem.azimuth_DX = ParameterUtils::deg2rad(geometry.moonAzimuth_TX_deg);
    moonEphem.elevation_DX = ParameterUtils::deg2rad(geometry.moonElevation_TX_deg);
    moonEphem.azimuth_Home = ParameterUtils::deg2rad(geometry.moonAzimuth_RX_deg);
    moonEphem.elevation_Home = ParameterUtils::deg2rad(geometry.moonElevation_RX_deg);
    moonEphem.ephemerisSource = geometry.ephemerisSource;

    moonEphem.hourAngle_DX = geometry.hourAngle_TX_rad;
    moonEphem.hourAngle_Home = geometry.hourAngle_RX_rad;

    m_faradayCalc.setMoonEphemeris(moonEphem);
}

PolarizationResults PolarizationModule::convertResults(
    const CalculationResults& faradayResults) {

    PolarizationResults results;

    results.spatialRotation_deg = faradayResults.spatialRotation_deg;
    results.faradayRotation_TX_deg = faradayResults.faradayRotation_DX_deg;
    results.faradayRotation_RX_deg = faradayResults.faradayRotation_Home_deg;
    results.totalRotation_deg = faradayResults.totalRotation_deg;
    results.PLF = faradayResults.PLF;
    results.polarizationLoss_dB = faradayResults.polarizationLoss_dB;
    results.polarizationEfficiency_percent = faradayResults.polarizationEfficiency;
    results.parallacticAngle_TX_deg = faradayResults.parallacticAngle_DX_deg;
    results.parallacticAngle_RX_deg = faradayResults.parallacticAngle_Home_deg;
    results.slantFactor_TX = faradayResults.slantFactor_DX;
    results.slantFactor_RX = faradayResults.slantFactor_Home;

    return results;
}

PolarizationResults PolarizationModule::calculate(
    const LinkBudgetParameters& params,
    const GeometryResults& geometry) {

    // Setup the Faraday calculator with current parameters
    setupCalculator(params, geometry);

    // Perform calculation
    CalculationResults faradayResults = m_faradayCalc.calculate();

    // Check for errors
    if (!faradayResults.calculationSuccess) {
        // Return empty results with error indication
        PolarizationResults errorResults;
        errorResults.PLF = 0.0;
        errorResults.polarizationLoss_dB = 999.0;  // Indicate error
        return errorResults;
    }

    // Convert and return results
    return convertResults(faradayResults);
}
