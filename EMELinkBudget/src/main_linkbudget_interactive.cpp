#define _CRT_SECURE_NO_WARNINGS  // Disable MSVC security warnings for standard C functions

#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <ctime>

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printSeparator(char c = '=', int length = 80) {
    std::cout << std::string(length, c) << std::endl;
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << std::endl;
    printSeparator();
}

bool getYesNo(const std::string& prompt) {
    std::string input;
    std::cout << prompt << " (y/n): ";
    std::getline(std::cin, input);
    return (input == "y" || input == "Y" || input == "yes" || input == "Yes");
}

double getDouble(const std::string& prompt, double defaultValue) {
    std::string input;
    std::cout << prompt << " [" << defaultValue << "]: ";
    std::getline(std::cin, input);

    if (input.empty()) {
        return defaultValue;
    }

    try {
        return std::stod(input);
    } catch (...) {
        std::cout << "Invalid input, using default: " << defaultValue << std::endl;
        return defaultValue;
    }
}

std::string getString(const std::string& prompt, const std::string& defaultValue) {
    std::string input;
    std::cout << prompt << " [" << defaultValue << "]: ";
    std::getline(std::cin, input);

    if (input.empty()) {
        return defaultValue;
    }

    return input;
}

void inputStationData(const std::string& stationName, SiteParameters& site) {
    printHeader(stationName + " Station Configuration");

    // Callsign
    site.callsign = getString("Callsign", "");

    // Grid locator
    std::string grid = getString("Maidenhead Grid Locator (e.g., OM81ks)", "");

    if (!grid.empty()) {
        try {
            double lat, lon;
            MaidenheadGrid::gridToLatLon(grid, lat, lon);
            site.latitude = ParameterUtils::deg2rad(lat);
            site.longitude = ParameterUtils::deg2rad(lon);
            site.gridLocator = grid;
            std::cout << "  => Latitude: " << std::fixed << std::setprecision(4)
                      << lat << " deg" << std::endl;
            std::cout << "  => Longitude: " << lon << " deg" << std::endl;
        } catch (...) {
            std::cout << "Invalid grid locator, using manual input..." << std::endl;
            double lat = getDouble("Latitude (degrees, -90 to 90)", 0.0);
            double lon = getDouble("Longitude (degrees, -180 to 180)", 0.0);
            site.latitude = ParameterUtils::deg2rad(lat);
            site.longitude = ParameterUtils::deg2rad(lon);
            site.gridLocator = MaidenheadGrid::latLonToGrid(lat, lon, 6);
        }
    } else {
        double lat = getDouble("Latitude (degrees, -90 to 90)", 0.0);
        double lon = getDouble("Longitude (degrees, -180 to 180)", 0.0);
        site.latitude = ParameterUtils::deg2rad(lat);
        site.longitude = ParameterUtils::deg2rad(lon);
        site.gridLocator = MaidenheadGrid::latLonToGrid(lat, lon, 6);
    }

    // Polarization
    std::cout << "\nPolarization Configuration:" << std::endl;
    std::cout << "  1. Linear Horizontal (psi=0, chi=0)" << std::endl;
    std::cout << "  2. Linear Vertical (psi=90, chi=0)" << std::endl;
    std::cout << "  3. RHCP (psi=0, chi=45)" << std::endl;
    std::cout << "  4. LHCP (psi=0, chi=-45)" << std::endl;
    std::cout << "  5. Custom" << std::endl;

    int polChoice = static_cast<int>(getDouble("Select polarization", 1.0));

    switch (polChoice) {
        case 1:
            site.psi = 0.0;
            site.chi = 0.0;
            break;
        case 2:
            site.psi = ParameterUtils::deg2rad(90.0);
            site.chi = 0.0;
            break;
        case 3:
            site.psi = 0.0;
            site.chi = ParameterUtils::deg2rad(45.0);
            break;
        case 4:
            site.psi = 0.0;
            site.chi = ParameterUtils::deg2rad(-45.0);
            break;
        case 5:
            site.psi = ParameterUtils::deg2rad(getDouble("Orientation angle psi (degrees)", 0.0));
            site.chi = ParameterUtils::deg2rad(getDouble("Ellipticity angle chi (degrees)", 0.0));
            break;
        default:
            site.psi = 0.0;
            site.chi = 0.0;
    }

    std::cout << "  => Polarization: " << ParameterUtils::getPolarizationType(site.chi) << std::endl;
    std::cout << std::endl;
}

void inputSystemConfiguration(LinkBudgetParameters& params) {
    printHeader("System Configuration");

    // Frequency
    std::cout << "Common frequencies:" << std::endl;
    std::cout << "  50 MHz (6m), 144 MHz (2m), 432 MHz (70cm)" << std::endl;
    std::cout << "  1296 MHz (23cm), 2400 MHz (13cm), 5760 MHz (6cm)" << std::endl;
    params.frequency_MHz = getDouble("Operating frequency (MHz)", 144.0);
    std::cout << "  => Band: " << ParameterUtils::getFrequencyBand(params.frequency_MHz) << std::endl;

    // TX Power
    std::cout << "\nTransmitter Configuration:" << std::endl;
    params.txPower_dBm = getDouble("TX Power (dBm, e.g., 50=100W, 40=10W)", 50.0);
    double txPower_W = std::pow(10.0, (params.txPower_dBm - 30.0) / 10.0);
    std::cout << "  => Power: " << std::fixed << std::setprecision(1) << txPower_W << " W" << std::endl;

    params.txGain_dBi = getDouble("TX Antenna Gain (dBi)", 20.0);
    params.txFeedlineLoss_dB = getDouble("TX Feedline Loss (dB)", 0.5);

    // RX Configuration
    std::cout << "\nReceiver Configuration:" << std::endl;
    params.rxGain_dBi = getDouble("RX Antenna Gain (dBi)", 20.0);
    params.rxFeedlineLoss_dB = getDouble("RX Feedline Loss (dB)", 0.5);
    params.rxNoiseFigure_dB = getDouble("RX Noise Figure (dB, typical LNA: 0.3-0.8)", 0.5);
    params.bandwidth_Hz = getDouble("Bandwidth (Hz, WSJT-X: 3200)", 3200);

    std::cout << std::endl;
}

std::time_t inputObservationTime() {
    printHeader("Observation Time");

    std::cout << "When do you want to calculate the link budget?" << std::endl;
    std::cout << "  1. Current time (now)" << std::endl;
    std::cout << "  2. Specify date and time" << std::endl;

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 2) {
        std::cout << "\nEnter observation time (UTC):" << std::endl;
        int year = static_cast<int>(getDouble("  Year", 2026.0));
        int month = static_cast<int>(getDouble("  Month (1-12)", 2.0));
        int day = static_cast<int>(getDouble("  Day (1-31)", 16.0));
        int hour = static_cast<int>(getDouble("  Hour (0-23)", 12.0));
        int minute = static_cast<int>(getDouble("  Minute (0-59)", 0.0));

        std::tm timeinfo = {};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = 0;

        std::time_t obsTime = std::mktime(&timeinfo);

        // Format time string safely
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC\n", &timeinfo);
        std::cout << "  -> Using: " << timeStr;

        return obsTime;
    } else {
        std::time_t now = std::time(nullptr);

        // Format time string safely
        std::tm* timeinfo = std::localtime(&now);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S\n", timeinfo);
        std::cout << "  -> Using current time: " << timeStr;

        return now;
    }
}

void inputMoonEphemeris(MoonEphemeris& moon, std::time_t observationTime) {
    printHeader("Moon Position Data");

    std::cout << "The program needs moon position data for accurate calculations." << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  1. Auto-fetch from JPL Horizons (requires internet)" << std::endl;
    std::cout << "  2. Use estimated position (less accurate)" << std::endl;
    std::cout << "  3. Manual input (if you have data from astronomy software)" << std::endl;

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 1) {
        std::cout << "\n[!] JPL Horizons integration not yet implemented." << std::endl;
        std::cout << "Falling back to estimated position...\n" << std::endl;
        choice = 2;
    }

    if (choice == 2) {
        // Use estimated position based on time
        // This is a very rough approximation - moon moves ~13° per day in RA
        std::cout << "Using estimated moon position (approximate)..." << std::endl;
        std::cout << "[!] Note: For accurate results, use real ephemeris data!" << std::endl;

        // Simple estimation: assume moon at a typical visible position
        moon.rightAscension = ParameterUtils::deg2rad(180.0);
        moon.declination = ParameterUtils::deg2rad(15.0);
        moon.distance_km = 384400.0;  // Average distance
        moon.hourAngle_DX = 0.0;
        moon.hourAngle_Home = 0.0;
        moon.ephemerisSource = "Estimated";

        std::cout << "  => RA: 180.0 deg (estimated)" << std::endl;
        std::cout << "  => DEC: 15.0 deg (estimated)" << std::endl;
        std::cout << "  => Distance: 384400 km (average)" << std::endl;
    } else if (choice == 3) {
        std::cout << "\nIf you have astronomy software (Stellarium, WSJT-X, etc.)," << std::endl;
        std::cout << "you can get accurate moon position data:\n" << std::endl;

        double ra = getDouble("Right Ascension (degrees, 0-360)", 180.0);
        double dec = getDouble("Declination (degrees, -90 to 90)", 15.0);
        double dist = getDouble("Distance (km, typical: 356000-406000)", 384400.0);

        moon.rightAscension = ParameterUtils::deg2rad(ra);
        moon.declination = ParameterUtils::deg2rad(dec);
        moon.distance_km = dist;
        moon.hourAngle_DX = 0.0;
        moon.hourAngle_Home = 0.0;
        moon.ephemerisSource = "Manual Input";
    }

    std::cout << std::endl;
}

void inputIonosphereData(IonosphereData& iono, std::time_t observationTime,
                         const SiteParameters& txSite, const SiteParameters& rxSite) {
    printHeader("Ionosphere Data");

    std::cout << "The program needs ionosphere data (TEC and magnetic field) for" << std::endl;
    std::cout << "accurate Faraday rotation calculations." << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  1. Auto-fetch from IONEX/GLOTEC (requires internet)" << std::endl;
    std::cout << "  2. Use typical values (less accurate)" << std::endl;
    std::cout << "  3. Manual input (if you have measured data)" << std::endl;

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 1) {
        std::cout << "\n[!] Real-time data fetching not yet implemented." << std::endl;
        std::cout << "Falling back to typical values...\n" << std::endl;
        choice = 2;
    }

    if (choice == 2) {
        std::cout << "Using typical ionosphere values..." << std::endl;
        std::cout << "[!] Note: Actual values vary by location, time, and solar activity!" << std::endl;

        // Estimate based on latitude (very rough)
        double lat_tx = ParameterUtils::rad2deg(txSite.latitude);
        double lat_rx = ParameterUtils::rad2deg(rxSite.latitude);

        // Typical TEC values: higher near equator, lower at poles
        // Daytime: 20-50 TECU, Nighttime: 5-20 TECU
        // We'll use moderate values
        iono.vTEC_DX = 25.0;
        iono.vTEC_Home = 25.0;

        // F2 layer height: typically 300-400 km
        iono.hmF2_DX = 350.0;
        iono.hmF2_Home = 350.0;

        // Magnetic field: varies by latitude
        // Equator: ~3e-5 T, Mid-latitudes: ~5e-5 T, Poles: ~6e-5 T
        iono.B_magnitude_DX = 5.0e-5;
        iono.B_magnitude_Home = 5.0e-5;

        // Magnetic inclination: varies by latitude
        // Equator: ~0°, Mid-latitudes: ~60°, Poles: ~90°
        double incl_tx = std::abs(lat_tx) * 1.2;  // Rough approximation
        double incl_rx = std::abs(lat_rx) * 1.2;
        if (incl_tx > 90.0) incl_tx = 90.0;
        if (incl_rx > 90.0) incl_rx = 90.0;

        iono.B_inclination_DX = ParameterUtils::deg2rad(incl_tx);
        iono.B_inclination_Home = ParameterUtils::deg2rad(incl_rx);

        // Declination: typically small, use 0
        iono.B_declination_DX = 0.0;
        iono.B_declination_Home = 0.0;

        iono.dataSource = "Typical Values";

        std::cout << "  TX Station:" << std::endl;
        std::cout << "    => TEC: " << iono.vTEC_DX << " TECU (typical)" << std::endl;
        std::cout << "    => Magnetic inclination: " << std::fixed << std::setprecision(1)
                  << incl_tx << " deg (estimated from latitude)" << std::endl;
        std::cout << "  RX Station:" << std::endl;
        std::cout << "    => TEC: " << iono.vTEC_Home << " TECU (typical)" << std::endl;
        std::cout << "    => Magnetic inclination: " << incl_rx << " deg (estimated from latitude)" << std::endl;

    } else if (choice == 3) {
        std::cout << "\nIf you have measured or downloaded ionosphere data:\n" << std::endl;

        std::cout << "TX Station Ionosphere:" << std::endl;
        iono.vTEC_DX = getDouble("  Vertical TEC (TECU, typical: 10-50)", 25.0);
        iono.hmF2_DX = getDouble("  F2 layer height (km, typical: 300-400)", 350.0);
        iono.B_magnitude_DX = getDouble("  Magnetic field (Tesla, typical: 3e-5 to 6e-5)", 5.0e-5);
        iono.B_inclination_DX = ParameterUtils::deg2rad(
            getDouble("  Magnetic inclination (degrees, 0=equator, 90=pole)", 60.0));
        iono.B_declination_DX = ParameterUtils::deg2rad(
            getDouble("  Magnetic declination (degrees)", 0.0));

        std::cout << "\nRX Station Ionosphere:" << std::endl;
        iono.vTEC_Home = getDouble("  Vertical TEC (TECU)", 25.0);
        iono.hmF2_Home = getDouble("  F2 layer height (km)", 350.0);
        iono.B_magnitude_Home = getDouble("  Magnetic field (Tesla)", 5.0e-5);
        iono.B_inclination_Home = ParameterUtils::deg2rad(
            getDouble("  Magnetic inclination (degrees)", 60.0));
        iono.B_declination_Home = ParameterUtils::deg2rad(
            getDouble("  Magnetic declination (degrees)", 0.0));

        iono.dataSource = "Manual Input";
    }

    std::cout << std::endl;
}

void displayResults(const LinkBudgetResults& results) {
    if (!results.calculationSuccess) {
        std::cout << "\n[X] Calculation Failed: " << results.errorMessage << std::endl;
        return;
    }

    printHeader("EME Link Budget Results");

    // Geometry
    std::cout << "\n[*] Geometry & Moon Position:" << std::endl;
    std::cout << "  Moon RA/DEC: " << std::fixed << std::setprecision(2)
              << results.geometry.moonRA_deg << " deg / "
              << results.geometry.moonDEC_deg << " deg" << std::endl;
    std::cout << "  Moon Distance: " << results.geometry.moonDistance_km << " km" << std::endl;
    std::cout << "  TX Elevation: " << results.geometry.moonElevation_TX_deg << " deg" << std::endl;
    std::cout << "  RX Elevation: " << results.geometry.moonElevation_RX_deg << " deg" << std::endl;
    std::cout << "  Path Length: " << results.geometry.totalPathLength_km << " km" << std::endl;

    // Path Loss
    std::cout << "\n[*] Path Loss Analysis:" << std::endl;
    std::cout << "  Free Space Loss: " << results.pathLoss.freeSpaceLoss_dB << " dB" << std::endl;
    std::cout << "  Lunar Scattering: " << results.pathLoss.lunarScatteringLoss_dB << " dB" << std::endl;
    std::cout << "  Atmospheric Loss: " << results.pathLoss.atmosphericLoss_Total_dB << " dB" << std::endl;
    std::cout << "  Total Path Loss: " << results.pathLoss.totalPathLoss_dB << " dB" << std::endl;

    // Polarization
    std::cout << "\n[*] Polarization Analysis:" << std::endl;
    std::cout << "  Spatial Rotation: " << std::setprecision(3)
              << results.polarization.spatialRotation_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (TX): " << results.polarization.faradayRotation_TX_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (RX): " << results.polarization.faradayRotation_RX_deg << " deg" << std::endl;
    std::cout << "  Total Rotation: " << results.polarization.totalRotation_deg << " deg" << std::endl;
    std::cout << "  Polarization Loss: " << std::setprecision(2)
              << results.polarization.polarizationLoss_dB << " dB" << std::endl;
    std::cout << "  PLF: " << std::setprecision(6) << results.polarization.PLF << std::endl;

    // Noise
    std::cout << "\n[*] Noise Analysis:" << std::endl;
    std::cout << "  Sky Noise: " << std::setprecision(1)
              << results.noise.skyNoiseTemp_K << " K" << std::endl;
    std::cout << "  Ground Spillover: " << results.noise.groundSpilloverTemp_K << " K" << std::endl;
    std::cout << "  System Noise: " << results.noise.systemNoiseTemp_K << " K" << std::endl;
    std::cout << "  Noise Power: " << std::setprecision(2)
              << results.noise.noisePower_dBm << " dBm" << std::endl;

    // SNR & Link Margin
    std::cout << "\n[*] Signal-to-Noise Ratio:" << std::endl;
    std::cout << "  Received Power: " << results.snr.receivedSignalPower_dBm << " dBm" << std::endl;
    std::cout << "  SNR: " << results.snr.SNR_dB << " dB" << std::endl;
    std::cout << "  Fading Margin: " << results.snr.fadingMargin_dB << " dB" << std::endl;
    std::cout << "  Effective SNR: " << results.snr.effectiveSNR_dB << " dB" << std::endl;
    std::cout << "  Required SNR: " << results.snr.requiredSNR_dB << " dB (WSJT-X)" << std::endl;

    // Link Margin
    printSeparator('-', 80);
    std::cout << "\n[*] LINK MARGIN: " << std::setprecision(2)
              << results.snr.linkMargin_dB << " dB" << std::endl;

    if (results.snr.linkViable) {
        std::cout << "[OK] Link Status: VIABLE - QSO possible!" << std::endl;
    } else {
        std::cout << "[X] Link Status: NOT VIABLE - Insufficient margin" << std::endl;
    }

    printSeparator();
}

int main() {
    printHeader("EME Link Budget Calculator - Interactive Mode");
    std::cout << "Complete EME Link Analysis with User Input\n" << std::endl;

    LinkBudgetParameters params;

    // Step 1: Observation Time
    params.observationTime = inputObservationTime();

    // Step 2: Input TX Station
    inputStationData("TX (DX)", params.txSite);

    // Step 3: Input RX Station
    inputStationData("RX (Home)", params.rxSite);

    // Step 4: Input System Configuration
    inputSystemConfiguration(params);

    // Step 5: Input Moon Ephemeris (needs observation time)
    inputMoonEphemeris(params.moonEphemeris, params.observationTime);

    // Step 6: Input Ionosphere Data (needs observation time and station locations)
    inputIonosphereData(params.ionosphereData, params.observationTime,
                        params.txSite, params.rxSite);

    // Step 7: Calculation Options
    printHeader("Calculation Options");
    std::cout << "Enable advanced physical effects (recommended: all yes):\n" << std::endl;
    params.includeFaradayRotation = getYesNo("Include Faraday rotation (ionosphere effect)");
    params.includeSpatialRotation = getYesNo("Include spatial rotation (geometry effect)");
    params.includeMoonReflection = getYesNo("Include moon reflection (polarization flip)");
    params.includeAtmosphericLoss = getYesNo("Include atmospheric loss");
    params.includeGroundSpillover = getYesNo("Include ground spillover noise");
    std::cout << std::endl;

    // Perform Calculation
    std::cout << "Calculating link budget..." << std::endl;
    EMELinkBudget linkBudget(params);
    LinkBudgetResults results = linkBudget.calculate();

    // Display Results
    displayResults(results);

    // Ask if user wants to calculate again
    std::cout << "\n";
    if (getYesNo("Calculate another link")) {
        std::cout << "\n\n";
        return main();  // Restart
    }

    std::cout << "\nThank you for using EME Link Budget Calculator!" << std::endl;
    std::cout << "\nTips for better accuracy:" << std::endl;
    std::cout << "  * Use real moon position from JPL Horizons or WSJT-X" << std::endl;
    std::cout << "  * Use real-time TEC data from IONEX files" << std::endl;
    std::cout << "  * Measure your actual system parameters" << std::endl;
    std::cout << "  * Check results during actual EME QSOs" << std::endl;

    return 0;
}
