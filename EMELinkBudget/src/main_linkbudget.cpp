#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include <iostream>
#include <iomanip>
#include <string>

void printSeparator(char c = '=', int length = 80) {
    std::cout << std::string(length, c) << std::endl;
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << std::endl;
    printSeparator();
}

void printSubHeader(const std::string& title) {
    std::cout << "\n" << title << std::endl;
    printSeparator('-', 80);
}

void printStationInfo(const std::string& name, const SiteParameters& site) {
    std::cout << name << " Station:" << std::endl;
    if (!site.callsign.empty()) {
        std::cout << "  Callsign: " << site.callsign << std::endl;
    }
    std::cout << "  Grid: " << site.gridLocator << std::endl;
    std::cout << "  Latitude: " << std::fixed << std::setprecision(4)
              << ParameterUtils::rad2deg(site.latitude) << " deg" << std::endl;
    std::cout << "  Longitude: "
              << ParameterUtils::rad2deg(site.longitude) << " deg" << std::endl;
    std::cout << "  Polarization: "
              << ParameterUtils::getPolarizationType(site.chi) << std::endl;
}

void printGeometryResults(const GeometryResults& geo) {
    printSubHeader("Geometry & Moon Position");
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Moon RA: " << geo.moonRA_deg << " deg" << std::endl;
    std::cout << "  Moon DEC: " << geo.moonDEC_deg << " deg" << std::endl;
    std::cout << "  Moon Distance: " << geo.moonDistance_km << " km" << std::endl;
    std::cout << "\n  TX Station View:" << std::endl;
    std::cout << "    Azimuth: " << geo.moonAzimuth_TX_deg << " deg" << std::endl;
    std::cout << "    Elevation: " << geo.moonElevation_TX_deg << " deg" << std::endl;
    std::cout << "  RX Station View:" << std::endl;
    std::cout << "    Azimuth: " << geo.moonAzimuth_RX_deg << " deg" << std::endl;
    std::cout << "    Elevation: " << geo.moonElevation_RX_deg << " deg" << std::endl;
    std::cout << "\n  Path Length: " << geo.totalPathLength_km << " km" << std::endl;
    std::cout << "  Doppler Shift: " << geo.dopplerShift_Hz << " Hz" << std::endl;
}

void printPathLossResults(const PathLossResults& loss) {
    printSubHeader("Path Loss Analysis");
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Wavelength: " << loss.wavelength_m << " m" << std::endl;
    std::cout << "  Free Space Loss: " << loss.freeSpaceLoss_dB << " dB" << std::endl;
    std::cout << "  Lunar Scattering Loss: " << loss.lunarScatteringLoss_dB << " dB" << std::endl;
    std::cout << "  Atmospheric Loss (TX): " << loss.atmosphericLoss_TX_dB << " dB" << std::endl;
    std::cout << "  Atmospheric Loss (RX): " << loss.atmosphericLoss_RX_dB << " dB" << std::endl;
    std::cout << "  Total Path Loss: " << loss.totalPathLoss_dB << " dB" << std::endl;
}

void printPolarizationResults(const PolarizationResults& pol) {
    printSubHeader("Polarization Analysis");
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  Spatial Rotation: " << pol.spatialRotation_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (TX): " << pol.faradayRotation_TX_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (RX): " << pol.faradayRotation_RX_deg << " deg" << std::endl;
    std::cout << "  Total Rotation: " << pol.totalRotation_deg << " deg" << std::endl;
    std::cout << "  PLF: " << std::setprecision(6) << pol.PLF << std::endl;
    std::cout << "  Polarization Loss: " << std::setprecision(2)
              << pol.polarizationLoss_dB << " dB" << std::endl;
    std::cout << "  Efficiency: " << pol.polarizationEfficiency_percent << " %" << std::endl;
}

void printNoiseResults(const NoiseResults& noise) {
    printSubHeader("Noise Analysis");
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Sky Noise Temperature: " << noise.skyNoiseTemp_K << " K" << std::endl;
    std::cout << "  Ground Spillover: " << noise.groundSpilloverTemp_K << " K" << std::endl;
    std::cout << "  Antenna Noise: " << noise.antennaNoiseTemp_K << " K" << std::endl;
    std::cout << "  Antenna Effective: " << noise.antennaEffectiveTemp_K << " K" << std::endl;
    std::cout << "  Receiver Noise: " << noise.receiverNoiseTemp_K << " K" << std::endl;
    std::cout << "  System Noise: " << noise.systemNoiseTemp_K << " K" << std::endl;
    std::cout << "  Noise Power: " << std::setprecision(2)
              << noise.noisePower_dBm << " dBm" << std::endl;
}

void printSNRResults(const SNRResults& snr) {
    printSubHeader("SNR & Link Margin");
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Received Signal Power: " << snr.receivedSignalPower_dBm << " dBm" << std::endl;
    std::cout << "  SNR: " << snr.SNR_dB << " dB" << std::endl;
    std::cout << "  Fading Margin: " << snr.fadingMargin_dB << " dB" << std::endl;
    std::cout << "  Effective SNR: " << snr.effectiveSNR_dB << " dB" << std::endl;
    std::cout << "  Required SNR: " << snr.requiredSNR_dB << " dB" << std::endl;
    std::cout << "  Link Margin: " << snr.linkMargin_dB << " dB" << std::endl;
    std::cout << "  Link Status: " << (snr.linkViable ? "VIABLE" : "NOT VIABLE") << std::endl;
}

int main() {
    printHeader("EME Link Budget Calculator - Complete System");
    std::cout << "Comprehensive EME Link Analysis with Signal and Noise Modeling\n" << std::endl;

    // ========== Setup Link Parameters ==========

    LinkBudgetParameters params;

    // System configuration
    params.frequency_MHz = 144.0;
    params.bandwidth_Hz = 2500.0;
    params.txPower_dBm = 50.0;  // 100W
    params.txGain_dBi = 20.0;   // ~4x17 Yagi array
    params.rxGain_dBi = 20.0;
    params.txFeedlineLoss_dB = 0.5;
    params.rxFeedlineLoss_dB = 0.5;
    params.rxNoiseFigure_dB = 0.5;  // Good LNA
    params.physicalTemp_K = 290.0;

    std::cout << "System Configuration:" << std::endl;
    std::cout << "  Frequency: " << params.frequency_MHz << " MHz ("
              << ParameterUtils::getFrequencyBand(params.frequency_MHz) << " band)" << std::endl;
    std::cout << "  TX Power: " << params.txPower_dBm << " dBm" << std::endl;
    std::cout << "  TX Gain: " << params.txGain_dBi << " dBi" << std::endl;
    std::cout << "  RX Gain: " << params.rxGain_dBi << " dBi" << std::endl;
    std::cout << "  RX NF: " << params.rxNoiseFigure_dB << " dB" << std::endl;
    std::cout << "  Bandwidth: " << params.bandwidth_Hz << " Hz" << std::endl;
    std::cout << std::endl;

    // TX Station (DX)
    std::string tx_grid = "FN20xa";
    MaidenheadGrid::gridToLatLon(tx_grid, params.txSite.latitude, params.txSite.longitude);
    params.txSite.latitude = ParameterUtils::deg2rad(params.txSite.latitude);
    params.txSite.longitude = ParameterUtils::deg2rad(params.txSite.longitude);
    params.txSite.gridLocator = tx_grid;
    params.txSite.callsign = "W1ABC";
    params.txSite.psi = 0.0;
    params.txSite.chi = 0.0;  // Linear polarization

    // RX Station (Home)
    std::string rx_grid = "PM95vr";
    MaidenheadGrid::gridToLatLon(rx_grid, params.rxSite.latitude, params.rxSite.longitude);
    params.rxSite.latitude = ParameterUtils::deg2rad(params.rxSite.latitude);
    params.rxSite.longitude = ParameterUtils::deg2rad(params.rxSite.longitude);
    params.rxSite.gridLocator = rx_grid;
    params.rxSite.callsign = "BG0AAA";
    params.rxSite.psi = 0.0;
    params.rxSite.chi = 0.0;  // Linear polarization

    printStationInfo("TX", params.txSite);
    std::cout << std::endl;
    printStationInfo("RX", params.rxSite);

    // Moon ephemeris (manual input for this example)
    params.moonEphemeris.rightAscension = ParameterUtils::deg2rad(180.0);
    params.moonEphemeris.declination = ParameterUtils::deg2rad(15.0);
    params.moonEphemeris.distance_km = 384400.0;
    params.moonEphemeris.hourAngle_DX = ParameterUtils::deg2rad(30.0);
    params.moonEphemeris.hourAngle_Home = ParameterUtils::deg2rad(45.0);
    params.moonEphemeris.ephemerisSource = "Manual Input";

    // Ionosphere data
    params.ionosphereData.vTEC_DX = 25.0;
    params.ionosphereData.vTEC_Home = 30.0;
    params.ionosphereData.hmF2_DX = 350.0;
    params.ionosphereData.hmF2_Home = 350.0;
    params.ionosphereData.B_magnitude_DX = 5.0e-5;
    params.ionosphereData.B_magnitude_Home = 4.8e-5;
    params.ionosphereData.B_inclination_DX = ParameterUtils::deg2rad(60.0);
    params.ionosphereData.B_inclination_Home = ParameterUtils::deg2rad(50.0);
    params.ionosphereData.dataSource = "Manual Input";

    // Calculation options
    params.includeFaradayRotation = true;
    params.includeSpatialRotation = true;
    params.includeMoonReflection = true;
    params.includeAtmosphericLoss = true;
    params.includeGroundSpillover = true;

    // ========== Perform Calculation ==========

    EMELinkBudget linkBudget(params);
    LinkBudgetResults results = linkBudget.calculate();

    if (!results.calculationSuccess) {
        std::cout << "\nError: " << results.errorMessage << std::endl;
        return 1;
    }

    // ========== Display Results ==========

    printGeometryResults(results.geometry);
    printPathLossResults(results.pathLoss);
    printPolarizationResults(results.polarization);
    printNoiseResults(results.noise);
    printSNRResults(results.snr);

    // ========== Summary ==========

    printSubHeader("Link Budget Summary");
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Total Path Loss: " << results.totalLoss_dB << " dB" << std::endl;
    std::cout << "  System Noise: " << results.noise.systemNoiseTemp_K << " K" << std::endl;
    std::cout << "  Link Margin: " << results.snr.linkMargin_dB << " dB" << std::endl;
    std::cout << "  Link Status: "
              << (results.snr.linkViable ? "✓ VIABLE" : "✗ NOT VIABLE") << std::endl;

    std::cout << "\n";
    printSeparator();
    std::cout << "Calculation Complete" << std::endl;
    printSeparator();

    return 0;
}
