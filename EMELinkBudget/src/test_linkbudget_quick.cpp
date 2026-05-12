#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

struct TestFailure {
    std::string name;
    std::string reason;
};

std::vector<TestFailure> g_failures;

bool approxEqual(double actual, double expected, double tol, const char* what,
                 const std::string& caseName) {
    const double diff = std::fabs(actual - expected);
    if (diff <= tol) {
        std::printf("  [OK] %-28s actual=%12.4f  expected=%12.4f  (tol=%g)\n",
                    what, actual, expected, tol);
        return true;
    }
    std::printf("  [FAIL] %-26s actual=%12.4f  expected=%12.4f  (diff=%g, tol=%g)\n",
                what, actual, expected, diff, tol);
    g_failures.push_back({caseName, std::string(what) +
                         ": actual=" + std::to_string(actual) +
                         " expected=" + std::to_string(expected)});
    return false;
}

// Expected values are captured at a known-good state and guard against
// silent regressions. Absolute physical references (ARRL / VK3UM EME calc):
//   - 144 MHz, d=384400 km echo loss ~ 252.1 dB
//   - 1296 MHz, d=384400 km echo loss ~ 271.3 dB
void runCase144MHzFN20_PM95() {
    const std::string caseName = "144 MHz FN20xa <-> PM95vr";
    std::printf("\n== %s ==\n", caseName.c_str());

    LinkBudgetParameters params;
    params.frequency_MHz = 144.0;
    params.bandwidth_Hz = 2500.0;
    params.txPower_dBm = 50.0;
    params.txGain_dBi = 20.0;
    params.rxGain_dBi = 20.0;
    params.txFeedlineLoss_dB = 0.5;
    params.rxFeedlineLoss_dB = 0.5;
    params.rxNoiseFigure_dB = 0.5;

    double lat, lon;
    MaidenheadGrid::gridToLatLon("FN20xa", lat, lon);
    params.txSite.latitude = ParameterUtils::deg2rad(lat);
    params.txSite.longitude = ParameterUtils::deg2rad(lon);
    params.txSite.gridLocator = "FN20xa";

    MaidenheadGrid::gridToLatLon("PM95vr", lat, lon);
    params.rxSite.latitude = ParameterUtils::deg2rad(lat);
    params.rxSite.longitude = ParameterUtils::deg2rad(lon);
    params.rxSite.gridLocator = "PM95vr";

    params.moonEphemeris.rightAscension = ParameterUtils::deg2rad(180.0);
    params.moonEphemeris.declination = ParameterUtils::deg2rad(15.0);
    params.moonEphemeris.distance_km = 384400.0;
    params.moonEphemeris.hourAngle_DX = ParameterUtils::deg2rad(30.0);
    params.moonEphemeris.hourAngle_Home = ParameterUtils::deg2rad(45.0);
    params.moonEphemeris.rangeRate_km_s = -0.5;

    params.ionosphereData.vTEC_DX = 25.0;
    params.ionosphereData.vTEC_Home = 30.0;
    params.ionosphereData.B_magnitude_DX = 5.0e-5;
    params.ionosphereData.B_magnitude_Home = 4.8e-5;
    params.ionosphereData.B_inclination_DX = ParameterUtils::deg2rad(60.0);
    params.ionosphereData.B_inclination_Home = ParameterUtils::deg2rad(50.0);

    EMELinkBudget linkBudget(params);
    LinkBudgetResults r = linkBudget.calculate();

    approxEqual(r.calculationSuccess ? 1.0 : 0.0, 1.0, 0.1, "calculationSuccess", caseName);
    approxEqual(r.pathLoss.totalPathLoss_dB, 251.98, 0.5, "pathLoss totalPathLoss_dB", caseName);
    approxEqual(r.pathLoss.freeSpaceLoss_dB, 251.97, 0.5, "pathLoss freeSpaceLoss_dB", caseName);
    approxEqual(r.polarization.polarizationLoss_dB, 1.20, 1.0, "polarization Loss_dB", caseName);
    approxEqual(r.noise.systemNoiseTemp_K, 326.7, 50.0, "systemNoiseTemp_K", caseName);
    approxEqual(r.geometry.dopplerShift_Hz, 480.5, 2.0, "dopplerShift_Hz (f*v/c)", caseName);
    approxEqual(r.geometry.moonDistance_km, 384400.0, 1.0, "moonDistance_km", caseName);
    approxEqual(r.snr.SNR_dB, -24.70, 2.0, "SNR_dB", caseName);
}

void runCase1296MHzMonostaticGeometry() {
    const std::string caseName = "1296 MHz path-loss only (monostatic limit)";
    std::printf("\n== %s ==\n", caseName.c_str());

    LinkBudgetParameters params;
    params.frequency_MHz = 1296.0;
    params.bandwidth_Hz = 500.0;
    params.txPower_dBm = 50.0;
    params.txGain_dBi = 30.0;
    params.rxGain_dBi = 30.0;
    params.txFeedlineLoss_dB = 0.5;
    params.rxFeedlineLoss_dB = 0.5;
    params.rxNoiseFigure_dB = 0.5;
    params.includeFaradayRotation = false;
    params.includeSpatialRotation = false;
    params.includeAtmosphericLoss = false;
    params.includeGroundSpillover = false;

    double lat, lon;
    MaidenheadGrid::gridToLatLon("FN20xa", lat, lon);
    params.txSite.latitude = ParameterUtils::deg2rad(lat);
    params.txSite.longitude = ParameterUtils::deg2rad(lon);
    params.rxSite.latitude = ParameterUtils::deg2rad(lat);
    params.rxSite.longitude = ParameterUtils::deg2rad(lon);

    params.moonEphemeris.rightAscension = ParameterUtils::deg2rad(0.0);
    params.moonEphemeris.declination = ParameterUtils::deg2rad(0.0);
    params.moonEphemeris.distance_km = 384400.0;
    params.moonEphemeris.hourAngle_DX = 0.0;
    params.moonEphemeris.hourAngle_Home = 0.0;
    params.moonEphemeris.rangeRate_km_s = 0.0;

    EMELinkBudget linkBudget(params);
    LinkBudgetResults r = linkBudget.calculate();

    // Analytical expectation for echo path loss at 1296 MHz, 384400 km:
    //   L = -14.46 + 20*log10(1296) + 40*log10(384400) = 271.27 dB
    approxEqual(r.calculationSuccess ? 1.0 : 0.0, 1.0, 0.1, "calculationSuccess", caseName);
    approxEqual(r.pathLoss.freeSpaceLoss_dB, 271.27, 0.5, "echo path loss (1296 MHz)", caseName);
    approxEqual(r.pathLoss.atmosphericLoss_Total_dB, 0.0, 1e-6,
                "atmosphericLoss disabled", caseName);
    approxEqual(r.geometry.dopplerShift_Hz, 0.0, 1e-6,
                "dopplerShift zero when rangeRate=0", caseName);
}

}  // namespace

int main() {
    std::printf("EME Link Budget regression tests\n");

    runCase144MHzFN20_PM95();
    runCase1296MHzMonostaticGeometry();

    std::printf("\n");
    if (g_failures.empty()) {
        std::printf("All regression tests PASSED.\n");
        return 0;
    }

    std::printf("FAILED: %zu assertion(s) below tolerance:\n", g_failures.size());
    for (const auto& f : g_failures) {
        std::printf("  - [%s] %s\n", f.name.c_str(), f.reason.c_str());
    }
    return 1;
}
