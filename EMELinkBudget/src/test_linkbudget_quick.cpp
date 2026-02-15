#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include <iostream>

int main() {
    std::cout << "EME Link Budget System - Quick Test\n" << std::endl;

    // Create simple test parameters
    LinkBudgetParameters params;
    params.frequency_MHz = 144.0;
    params.bandwidth_Hz = 2500.0;
    params.txPower_dBm = 50.0;
    params.txGain_dBi = 20.0;
    params.rxGain_dBi = 20.0;
    params.txFeedlineLoss_dB = 0.5;
    params.rxFeedlineLoss_dB = 0.5;
    params.rxNoiseFigure_dB = 0.5;

    // Setup stations
    double lat, lon;
    MaidenheadGrid::gridToLatLon("FN20xa", lat, lon);
    params.txSite.latitude = ParameterUtils::deg2rad(lat);
    params.txSite.longitude = ParameterUtils::deg2rad(lon);
    params.txSite.gridLocator = "FN20xa";

    MaidenheadGrid::gridToLatLon("PM95vr", lat, lon);
    params.rxSite.latitude = ParameterUtils::deg2rad(lat);
    params.rxSite.longitude = ParameterUtils::deg2rad(lon);
    params.rxSite.gridLocator = "PM95vr";

    // Setup moon ephemeris
    params.moonEphemeris.rightAscension = ParameterUtils::deg2rad(180.0);
    params.moonEphemeris.declination = ParameterUtils::deg2rad(15.0);
    params.moonEphemeris.distance_km = 384400.0;
    params.moonEphemeris.hourAngle_DX = ParameterUtils::deg2rad(30.0);
    params.moonEphemeris.hourAngle_Home = ParameterUtils::deg2rad(45.0);

    // Setup ionosphere
    params.ionosphereData.vTEC_DX = 25.0;
    params.ionosphereData.vTEC_Home = 30.0;
    params.ionosphereData.B_magnitude_DX = 5.0e-5;
    params.ionosphereData.B_magnitude_Home = 4.8e-5;
    params.ionosphereData.B_inclination_DX = ParameterUtils::deg2rad(60.0);
    params.ionosphereData.B_inclination_Home = ParameterUtils::deg2rad(50.0);

    // Calculate
    EMELinkBudget linkBudget(params);
    LinkBudgetResults results = linkBudget.calculate();

    // Display results
    if (results.calculationSuccess) {
        std::cout << "✓ Calculation successful!\n" << std::endl;
        std::cout << "Path Loss: " << results.pathLoss.totalPathLoss_dB << " dB" << std::endl;
        std::cout << "Polarization Loss: " << results.polarization.polarizationLoss_dB << " dB" << std::endl;
        std::cout << "System Noise: " << results.noise.systemNoiseTemp_K << " K" << std::endl;
        std::cout << "SNR: " << results.snr.SNR_dB << " dB" << std::endl;
        std::cout << "Link Margin: " << results.snr.linkMargin_dB << " dB" << std::endl;
        std::cout << "Link Status: " << (results.snr.linkViable ? "VIABLE" : "NOT VIABLE") << std::endl;
    } else {
        std::cout << "✗ Calculation failed: " << results.errorMessage << std::endl;
        return 1;
    }

    return 0;
}
