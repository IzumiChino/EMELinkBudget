#include "FaradayRotation.h"
#include "Parameters.h"
#include "MaidenheadGrid.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

void printSeparator(char c = '=', int length = 75) {
    std::cout << std::string(length, c) << '\n';
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << '\n';
    printSeparator();
}

void printSubHeader(const std::string& title) {
    std::cout << "\n" << title << '\n';
    printSeparator('-', 75);
}

void printSiteInfo(const std::string& name, const SiteParameters& site) {
    std::cout << name << " Station Info:" << '\n';
    if (!site.callsign.empty()) {
        std::cout << "  Callsign: " << site.callsign << '\n';
    }
    std::cout << "  Grid: " << site.gridLocator << '\n';
    std::cout << "  Latitude: "
              << std::fixed << std::setprecision(4)
              << ParameterUtils::rad2deg(site.latitude) << " deg" << '\n';
    std::cout << "  Longitude: "
              << ParameterUtils::rad2deg(site.longitude) << " deg" << '\n';
    std::cout << "  Orientation psi: "
              << ParameterUtils::rad2deg(site.psi) << " deg" << '\n';
    std::cout << "  Ellipticity chi: "
              << ParameterUtils::rad2deg(site.chi) << " deg" << '\n';
    std::cout << "  Polarization: "
              << ParameterUtils::getPolarizationType(site.chi) << '\n';
}

void printResults(const CalculationResults& results) {
    if (!results.calculationSuccess) {
        std::cout << "\nError: " << results.errorMessage << '\n';
        return;
    }

    printSubHeader("Intermediate Values");
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  DX Parallactic Angle: " << results.parallacticAngle_DX_deg << " deg" << '\n';
    std::cout << "  Home Parallactic Angle: " << results.parallacticAngle_Home_deg << " deg" << '\n';
    std::cout << "  Spatial Rotation: " << results.spatialRotation_deg << " deg" << '\n';
    std::cout << "  DX Faraday Rotation: " << results.faradayRotation_DX_deg << " deg" << '\n';
    std::cout << "  Home Faraday Rotation: " << results.faradayRotation_Home_deg << " deg" << '\n';
    std::cout << "  Total Rotation: " << results.totalRotation_deg << " deg" << '\n';
    std::cout << "  DX Slant Factor: " << results.slantFactor_DX << '\n';
    std::cout << "  Home Slant Factor: " << results.slantFactor_Home << '\n';

    printSubHeader("Link Parameters");
    std::cout << "  Path Length: " << std::fixed << std::setprecision(1)
              << results.pathLength_km << " km" << '\n';
    std::cout << "  Propagation Delay: " << std::fixed << std::setprecision(3)
              << results.propagationDelay_ms << " ms" << '\n';

    printSubHeader("Polarization Loss Results");
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  PLF: " << results.PLF << '\n';
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  Loss: " << results.polarizationLoss_dB << " dB" << '\n';
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Efficiency: " << results.polarizationEfficiency << " %" << '\n';
}

int main() {
    std::cout << std::fixed << std::setprecision(3);

    printHeader("EME Faraday Rotation Polarization Loss Calculator V2");
    std::cout << "Faraday Rotation Polarization Loss Calculator V2 for EME\n" << '\n';

    printHeader("Example 1: Using Maidenhead Grid Locator");

    SystemConfiguration config;
    config.frequency_MHz = 144.0;
    config.includeFaradayRotation = true;
    config.includeSpatialRotation = true;
    config.includeMoonReflection = true;

    std::cout << "Frequency: " << config.frequency_MHz << " MHz ("
              << ParameterUtils::getFrequencyBand(config.frequency_MHz) << " band)" << '\n';
    std::cout << '\n';

    FaradayRotation calculator(config);

    std::string dx_grid = "FN20xa";
    calculator.setDXStationByGrid(
        dx_grid,
        ParameterUtils::deg2rad(0.0),
        ParameterUtils::deg2rad(0.0)
    );

    std::string home_grid = "PM95vr";
    calculator.setHomeStationByGrid(
        home_grid,
        ParameterUtils::deg2rad(0.0),
        ParameterUtils::deg2rad(0.0)
    );

    printSiteInfo("DX", calculator.getDXStation());
    std::cout << '\n';
    printSiteInfo("Home", calculator.getHomeStation());
    std::cout << '\n';

    double distance = calculator.calculateStationDistance();
    std::cout << "Ground Distance: " << std::fixed << std::setprecision(1)
              << distance << " km" << '\n';

    IonosphereData iono;
    iono.vTEC_DX = 25.0;
    iono.vTEC_Home = 30.0;
    iono.B_magnitude_DX = 5.0e-5;
    iono.B_magnitude_Home = 4.8e-5;
    iono.B_inclination_DX = ParameterUtils::deg2rad(60.0);
    iono.B_inclination_Home = ParameterUtils::deg2rad(50.0);
    iono.dataSource = "Manual Input";
    calculator.setIonosphereData(iono);

    printSubHeader("Ionosphere Parameters");
    std::cout << "  DX vTEC: " << iono.vTEC_DX << " TECU" << '\n';
    std::cout << "  Home vTEC: " << iono.vTEC_Home << " TECU" << '\n';
    std::cout << "  DX B field: " << iono.B_magnitude_DX * 1e6 << " uT" << '\n';
    std::cout << "  Home B field: " << iono.B_magnitude_Home * 1e6 << " uT" << '\n';

    MoonEphemeris moon;
    moon.declination = ParameterUtils::deg2rad(15.0);
    moon.hourAngle_DX = ParameterUtils::deg2rad(30.0);
    moon.hourAngle_Home = ParameterUtils::deg2rad(45.0);
    moon.distance_km = 384400.0;
    moon.ephemerisSource = "Manual Input";
    calculator.setMoonEphemeris(moon);

    printSubHeader("Moon Ephemeris");
    std::cout << "  Declination: " << ParameterUtils::rad2deg(moon.declination) << " deg" << '\n';
    std::cout << "  DX Hour Angle: " << ParameterUtils::rad2deg(moon.hourAngle_DX) << " deg" << '\n';
    std::cout << "  Home Hour Angle: " << ParameterUtils::rad2deg(moon.hourAngle_Home) << " deg" << '\n';
    std::cout << "  Distance: " << moon.distance_km << " km" << '\n';

    CalculationResults results = calculator.calculate();

    printResults(results);

    std::cout << "\n\n";
    printHeader("Example 2: Frequency Sweep Analysis");

    std::cout << std::setw(12) << "Freq(MHz)"
              << std::setw(12) << "Band"
              << std::setw(18) << "Faraday Rot(deg)"
              << std::setw(12) << "PLF"
              << std::setw(15) << "Loss(dB)" << '\n';
    printSeparator('-', 75);

    std::vector<double> frequencies = {50.0, 144.0, 432.0, 1296.0};

    for (double freq : frequencies) {
        SystemConfiguration freqConfig = config;
        freqConfig.frequency_MHz = freq;

        FaradayRotation calc(freqConfig);
        calc.setDXStationByGrid(dx_grid, 0.0, 0.0);
        calc.setHomeStationByGrid(home_grid, 0.0, 0.0);
        calc.setIonosphereData(iono);
        calc.setMoonEphemeris(moon);

        CalculationResults res = calc.calculate();

        if (res.calculationSuccess) {
            double totalFaraday = res.faradayRotation_DX_deg + res.faradayRotation_Home_deg;

            std::cout << std::setw(12) << freq
                      << std::setw(12) << ParameterUtils::getFrequencyBand(freq)
                      << std::setw(18) << std::fixed << std::setprecision(3) << totalFaraday
                      << std::setw(12) << std::setprecision(6) << res.PLF
                      << std::setw(15) << std::setprecision(3) << res.polarizationLoss_dB
                      << '\n';
        }
    }

    std::cout << "\n\n";
    printHeader("Example 3: Polarization Type Comparison");

    struct PolarizationTest {
        std::string name;
        double chi_deg;
    };

    std::vector<PolarizationTest> polarizations = {
        {"Linear H", 0.0},
        {"RHCP", 45.0},
        {"LHCP", -45.0},
        {"Elliptical", 30.0}
    };

    std::cout << std::setw(20) << "Polarization"
              << std::setw(15) << "chi (deg)"
              << std::setw(12) << "PLF"
              << std::setw(15) << "Loss(dB)" << '\n';
    printSeparator('-', 75);

    for (const auto& pol : polarizations) {
        FaradayRotation calc(config);
        calc.setDXStationByGrid(dx_grid, 0.0, ParameterUtils::deg2rad(pol.chi_deg));
        calc.setHomeStationByGrid(home_grid, 0.0, ParameterUtils::deg2rad(pol.chi_deg));
        calc.setIonosphereData(iono);
        calc.setMoonEphemeris(moon);

        CalculationResults res = calc.calculate();

        if (res.calculationSuccess) {
            std::cout << std::setw(20) << pol.name
                      << std::setw(15) << std::fixed << std::setprecision(1) << pol.chi_deg
                      << std::setw(12) << std::setprecision(6) << res.PLF
                      << std::setw(15) << std::setprecision(3) << res.polarizationLoss_dB
                      << '\n';
        }
    }

    std::cout << "\n";
    printSeparator();
    std::cout << "Calculation Complete" << '\n';
    printSeparator();

    return 0;
}
