#include "FaradayRotation.h"
#include "Parameters.h"
#include "MaidenheadGrid.h"
#include "IonosphereDataProvider.h"
#include "MoonCalendarReader.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <fstream>

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printSeparator(char c = '=', int length = 75) {
    std::cout << std::string(length, c) << std::endl;
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << std::endl;
    printSeparator();
}

int main() {
    std::cout << std::fixed << std::setprecision(3);

    printHeader("EME Faraday Rotation Calculator - Interactive Mode");
    std::cout << "\nThis program calculates polarization loss due to Faraday rotation\n";
    std::cout << "in Earth-Moon-Earth (EME) communications.\n" << std::endl;

    // ========== Input: Frequency ==========
    double frequency_MHz;
    std::cout << "Enter operating frequency (MHz): ";
    std::cin >> frequency_MHz;
    clearInputBuffer();

    SystemConfiguration config;
    config.frequency_MHz = frequency_MHz;
    config.includeFaradayRotation = true;
    config.includeSpatialRotation = true;
    config.includeMoonReflection = true;

    FaradayRotation calculator(config);

    // ========== Input: DX Station ==========
    std::cout << "\n--- DX Station Configuration ---" << std::endl;
    std::cout << "Enter DX station grid locator (e.g., FN20xa): ";
    std::string dx_grid;
    std::cin >> dx_grid;
    clearInputBuffer();

    std::cout << "Enter DX antenna orientation angle psi (degrees, 0=horizontal): ";
    double dx_psi;
    std::cin >> dx_psi;
    clearInputBuffer();

    std::cout << "Enter DX antenna ellipticity chi (degrees, 0=linear, 45=RHCP, -45=LHCP): ";
    double dx_chi;
    std::cin >> dx_chi;
    clearInputBuffer();

    try {
        calculator.setDXStationByGrid(dx_grid, ParameterUtils::deg2rad(dx_psi), ParameterUtils::deg2rad(dx_chi));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // ========== Input: Home Station ==========
    std::cout << "\n--- Home Station Configuration ---" << std::endl;
    std::cout << "Enter Home station grid locator (e.g., PM95vr): ";
    std::string home_grid;
    std::cin >> home_grid;
    clearInputBuffer();

    std::cout << "Enter Home antenna orientation angle psi (degrees, 0=horizontal): ";
    double home_psi;
    std::cin >> home_psi;
    clearInputBuffer();

    std::cout << "Enter Home antenna ellipticity chi (degrees, 0=linear, 45=RHCP, -45=LHCP): ";
    double home_chi;
    std::cin >> home_chi;
    clearInputBuffer();

    try {
        calculator.setHomeStationByGrid(home_grid, ParameterUtils::deg2rad(home_psi), ParameterUtils::deg2rad(home_chi));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // ========== Input: Ionosphere Parameters ==========
    std::cout << "\n--- Ionosphere Parameters ---" << std::endl;
    std::cout << "Data source options:\n";
    std::cout << "  1. Load from IONEX file (data.txt)\n";
    std::cout << "  2. Use default values (vTEC=25 TECU, B=50uT, inclination=60deg)\n";
    std::cout << "  3. Manual input\n";
    std::cout << "Select option (1/2/3): ";

    int iono_option;
    std::cin >> iono_option;
    clearInputBuffer();

    IonosphereData iono;
    std::tm obs_time = {};

    if (iono_option == 1) {
        IonosphereDataProvider provider;
        std::cout << "Loading IONEX file (data.txt)..." << std::endl;

        if (!provider.loadIonexFile("../data/data.txt")) {
            std::cerr << "Error: Could not load data.txt" << std::endl;
            std::cerr << "Falling back to default values." << std::endl;
            iono.vTEC_DX = 25.0;
            iono.vTEC_Home = 25.0;
            iono.hmF2_DX = 350.0;
            iono.hmF2_Home = 350.0;
            iono.B_magnitude_DX = 5.0e-5;
            iono.B_magnitude_Home = 5.0e-5;
            iono.B_inclination_DX = ParameterUtils::deg2rad(60.0);
            iono.B_inclination_Home = ParameterUtils::deg2rad(60.0);
        } else {
            std::cout << "IONEX file loaded successfully!" << std::endl;

            std::cout << "Loading WMM model (WMMHR.COF)..." << std::endl;
            if (!provider.loadWMMFile("../data/WMMHR.COF")) {
                std::cout << "Warning: Could not load WMM file. Using default magnetic field values." << std::endl;
            } else {
                std::cout << "WMM model loaded successfully!" << std::endl;
            }

            std::cout << "\nEnter observation date and time (UTC):" << std::endl;
            std::cout << "Year (e.g., 2026): ";
            int year;
            std::cin >> year;
            obs_time.tm_year = year - 1900;
            clearInputBuffer();

            std::cout << "Month (1-12): ";
            int month;
            std::cin >> month;
            obs_time.tm_mon = month - 1;
            clearInputBuffer();

            std::cout << "Day (1-31): ";
            std::cin >> obs_time.tm_mday;
            clearInputBuffer();

            std::cout << "Hour (0-23): ";
            std::cin >> obs_time.tm_hour;
            clearInputBuffer();

            std::cout << "Minute (0-59): ";
            std::cin >> obs_time.tm_min;
            clearInputBuffer();

            if (obs_time.tm_mday == 14 && month == 1)
				std::cout << "Happy Birthday Mutsumi Wakaba!" << std::endl;

            obs_time.tm_sec = 0;
            obs_time.tm_isdst = -1;

            double lat_dx = ParameterUtils::rad2deg(calculator.getDXStation().latitude);
            double lon_dx = ParameterUtils::rad2deg(calculator.getDXStation().longitude);
            double lat_home = ParameterUtils::rad2deg(calculator.getHomeStation().latitude);
            double lon_home = ParameterUtils::rad2deg(calculator.getHomeStation().longitude);

            double height_dx_km = 0.0;
            double height_home_km = 0.0;

            if (provider.getIonosphereData(obs_time, lat_dx, lon_dx, height_dx_km,
                                          lat_home, lon_home, height_home_km, iono)) {
                std::cout << "\nIonosphere data retrieved:" << std::endl;
                std::cout << "  DX vTEC: " << iono.vTEC_DX << " TECU" << std::endl;
                std::cout << "  Home vTEC: " << iono.vTEC_Home << " TECU" << std::endl;
                if (provider.isWMMLoaded()) {
                    std::cout << "  DX Magnetic Field: " << iono.B_magnitude_DX * 1e9 << " nT" << std::endl;
                    std::cout << "  DX Inclination: " << ParameterUtils::rad2deg(iono.B_inclination_DX) << " deg" << std::endl;
                    std::cout << "  DX Declination: " << ParameterUtils::rad2deg(iono.B_declination_DX) << " deg" << std::endl;
                    std::cout << "  Home Magnetic Field: " << iono.B_magnitude_Home * 1e9 << " nT" << std::endl;
                    std::cout << "  Home Inclination: " << ParameterUtils::rad2deg(iono.B_inclination_Home) << " deg" << std::endl;
                    std::cout << "  Home Declination: " << ParameterUtils::rad2deg(iono.B_declination_Home) << " deg" << std::endl;
                }
            } else {
                std::cerr << "Error: Could not retrieve TEC data for specified time/location" << std::endl;
                std::cerr << "Falling back to default values." << std::endl;
                iono.vTEC_DX = 25.0;
                iono.vTEC_Home = 25.0;
                iono.hmF2_DX = 350.0;
                iono.hmF2_Home = 350.0;
                iono.B_magnitude_DX = 5.0e-5;
                iono.B_magnitude_Home = 5.0e-5;
                iono.B_inclination_DX = ParameterUtils::deg2rad(60.0);
                iono.B_inclination_Home = ParameterUtils::deg2rad(60.0);
            }
        }
    } else if (iono_option == 2) {
        iono.vTEC_DX = 25.0;
        iono.vTEC_Home = 25.0;
        iono.hmF2_DX = 350.0;
        iono.hmF2_Home = 350.0;
        iono.B_magnitude_DX = 5.0e-5;
        iono.B_magnitude_Home = 5.0e-5;
        iono.B_inclination_DX = ParameterUtils::deg2rad(60.0);
        iono.B_inclination_Home = ParameterUtils::deg2rad(60.0);
        std::cout << "Using default values (vTEC=25 TECU, hmF2=350km, B=50uT, inclination=60deg)" << std::endl;
    } else {
        std::cout << "Enter DX station vTEC (TECU, typical: 10-50): ";
        std::cin >> iono.vTEC_DX;
        clearInputBuffer();

        std::cout << "Enter Home station vTEC (TECU, typical: 10-50): ";
        std::cin >> iono.vTEC_Home;
        clearInputBuffer();

        std::cout << "Enter DX station hmF2 peak height (km, typical: 250-400): ";
        std::cin >> iono.hmF2_DX;
        clearInputBuffer();

        std::cout << "Enter Home station hmF2 peak height (km, typical: 250-400): ";
        std::cin >> iono.hmF2_Home;
        clearInputBuffer();

        std::cout << "Enter DX magnetic field strength (uT, typical: 30-60): ";
        double B_DX_uT;
        std::cin >> B_DX_uT;
        clearInputBuffer();
        iono.B_magnitude_DX = B_DX_uT * 1e-6;

        std::cout << "Enter Home magnetic field strength (uT, typical: 30-60): ";
        double B_Home_uT;
        std::cin >> B_Home_uT;
        clearInputBuffer();
        iono.B_magnitude_Home = B_Home_uT * 1e-6;

        std::cout << "Enter DX magnetic inclination (degrees, typical: 50-70): ";
        double B_incl_DX;
        std::cin >> B_incl_DX;
        clearInputBuffer();
        iono.B_inclination_DX = ParameterUtils::deg2rad(B_incl_DX);

        std::cout << "Enter Home magnetic inclination (degrees, typical: 50-70): ";
        double B_incl_Home;
        std::cin >> B_incl_Home;
        clearInputBuffer();
        iono.B_inclination_Home = ParameterUtils::deg2rad(B_incl_Home);
    }
    calculator.setIonosphereData(iono);

    // ========== Input: Moon Ephemeris ==========
    std::cout << "\n--- Moon Ephemeris ---" << std::endl;
    std::cout << "Do you have moon elevation/azimuth data? (y/n): ";
    char have_elev;
    std::cin >> have_elev;
    clearInputBuffer();

    MoonEphemeris moon;

    if (have_elev == 'y' || have_elev == 'Y') {
        std::cout << "Enter DX station moon elevation (degrees above horizon): ";
        double elev_dx;
        std::cin >> elev_dx;
        clearInputBuffer();

        std::cout << "Enter DX station moon azimuth (degrees, 0=North, 90=East): ";
        double az_dx;
        std::cin >> az_dx;
        clearInputBuffer();

        std::cout << "Enter Home station moon elevation (degrees above horizon): ";
        double elev_home;
        std::cin >> elev_home;
        clearInputBuffer();

        std::cout << "Enter Home station moon azimuth (degrees, 0=North, 90=East): ";
        double az_home;
        std::cin >> az_home;
        clearInputBuffer();

        moon.elevation_DX = ParameterUtils::deg2rad(elev_dx);
        moon.azimuth_DX = ParameterUtils::deg2rad(az_dx);
        moon.elevation_Home = ParameterUtils::deg2rad(elev_home);
        moon.azimuth_Home = ParameterUtils::deg2rad(az_home);

        std::cout << "\nMoon declination options:\n";
        std::cout << "  1. Load from calendar.dat (automatic)\n";
        std::cout << "  2. Manual input\n";
        std::cout << "Select option (1/2): ";
        int decl_option;
        std::cin >> decl_option;
        clearInputBuffer();

        double moon_dec = 0.0;

        if (decl_option == 1) {
            MoonCalendarReader calendar;
            if (calendar.loadCalendarFile("../data/calendar.dat")) {
                std::tm date_only = obs_time;
                date_only.tm_hour = 0;
                date_only.tm_min = 0;
                date_only.tm_sec = 0;

                if (calendar.getMoonDeclination(date_only, moon_dec)) {
                    std::cout << "Moon declination from calendar: " << moon_dec << " deg" << std::endl;
                } else {
                    std::cout << "Could not find declination in calendar. Please enter manually: ";
                    std::cin >> moon_dec;
                    clearInputBuffer();
                }
            } else {
                std::cout << "Error: Could not load calendar.dat (from ../data/). Please enter manually: ";
                std::cin >> moon_dec;
                clearInputBuffer();
            }
        } else {
            std::cout << "Enter moon declination (degrees, typical: -28 to +28): ";
            std::cin >> moon_dec;
            clearInputBuffer();
        }

        moon.declination = ParameterUtils::deg2rad(moon_dec);

        // Calculate hour angle from elevation (approximate)
        double sinLat_DX = std::sin(calculator.getDXStation().latitude);
        double cosLat_DX = std::cos(calculator.getDXStation().latitude);
        double sinDec = std::sin(moon.declination);
        double cosDec = std::cos(moon.declination);
        double sinElev_DX = std::sin(moon.elevation_DX);

        double cosH_DX = (sinElev_DX - sinLat_DX * sinDec) / (cosLat_DX * cosDec);
        if (cosH_DX >= -1.0 && cosH_DX <= 1.0) {
            moon.hourAngle_DX = std::acos(cosH_DX);
            if (az_dx > 180.0) moon.hourAngle_DX = -moon.hourAngle_DX;
        } else {
            moon.hourAngle_DX = 0.0;
        }

        double sinLat_Home = std::sin(calculator.getHomeStation().latitude);
        double cosLat_Home = std::cos(calculator.getHomeStation().latitude);
        double sinElev_Home = std::sin(moon.elevation_Home);

        double cosH_Home = (sinElev_Home - sinLat_Home * sinDec) / (cosLat_Home * cosDec);
        if (cosH_Home >= -1.0 && cosH_Home <= 1.0) {
            moon.hourAngle_Home = std::acos(cosH_Home);
            if (az_home > 180.0) moon.hourAngle_Home = -moon.hourAngle_Home;
        } else {
            moon.hourAngle_Home = 0.0;
        }

    } else {
        std::cout << "\nMoon declination options:\n";
        std::cout << "  1. Load from calendar.dat (automatic)\n";
        std::cout << "  2. Manual input\n";
        std::cout << "Select option (1/2): ";
        int decl_option;
        std::cin >> decl_option;
        clearInputBuffer();

        double moon_dec = 0.0;

        if (decl_option == 1) {
            MoonCalendarReader calendar;
            if (calendar.loadCalendarFile("../data/calendar.dat")) {
                std::tm date_only = obs_time;
                date_only.tm_hour = 0;
                date_only.tm_min = 0;
                date_only.tm_sec = 0;

                if (calendar.getMoonDeclination(date_only, moon_dec)) {
                    std::cout << "Moon declination from calendar: " << moon_dec << " deg" << std::endl;
                } else {
                    std::cout << "Could not find declination in calendar. Please enter manually: ";
                    std::cin >> moon_dec;
                    clearInputBuffer();
                }
            } else {
                std::cout << "Error: Could not load calendar.dat (from ../data/). Please enter manually: ";
                std::cin >> moon_dec;
                clearInputBuffer();
            }
        } else {
            std::cout << "Enter moon declination (degrees, typical: -28 to +28): ";
            std::cin >> moon_dec;
            clearInputBuffer();
        }

        std::cout << "\nEnter DX station hour angle (degrees, 0=transit): ";
        double hour_angle_dx;
        std::cin >> hour_angle_dx;
        clearInputBuffer();

        std::cout << "Enter Home station hour angle (degrees, 0=transit): ";
        double hour_angle_home;
        std::cin >> hour_angle_home;
        clearInputBuffer();

        moon.declination = ParameterUtils::deg2rad(moon_dec);
        moon.hourAngle_DX = ParameterUtils::deg2rad(hour_angle_dx);
        moon.hourAngle_Home = ParameterUtils::deg2rad(hour_angle_home);
    }

    std::cout << "Enter Earth-Moon distance (km, typical: 356500-406700, default=384400): ";
    double moon_distance;
    std::cin >> moon_distance;
    clearInputBuffer();

    moon.distance_km = moon_distance;
    calculator.setMoonEphemeris(moon);

    // ========== Calculate ==========
    std::cout << "\nCalculating..." << std::endl;
    CalculationResults results = calculator.calculate();

    // Debug: Show calculated elevations
    const MoonEphemeris& moon_data = calculator.getMoonEphemeris();
    std::cout << "\nDebug - Calculated Moon Elevations:" << std::endl;
    std::cout << "  DX Elevation: " << ParameterUtils::rad2deg(moon_data.elevation_DX) << " deg" << std::endl;
    std::cout << "  Home Elevation: " << ParameterUtils::rad2deg(moon_data.elevation_Home) << " deg" << std::endl;

    // ========== Display Results ==========
    printHeader("Calculation Results");

    if (!results.calculationSuccess) {
        std::cerr << "Error: " << results.errorMessage << std::endl;
        return 1;
    }

    std::cout << "\n--- Station Information ---" << std::endl;
    std::cout << "DX Grid: " << calculator.getDXStation().gridLocator << std::endl;
    std::cout << "Home Grid: " << calculator.getHomeStation().gridLocator << std::endl;
    std::cout << "Ground Distance: " << std::fixed << std::setprecision(1)
              << calculator.calculateStationDistance() << " km" << std::endl;
    std::cout << "Frequency: " << frequency_MHz << " MHz ("
              << ParameterUtils::getFrequencyBand(frequency_MHz) << " band)" << std::endl;

    std::cout << "\n--- Rotation Components ---" << std::endl;
    std::cout << "Spatial Rotation: " << std::setprecision(3)
              << results.spatialRotation_deg << " deg" << std::endl;
    std::cout << "DX Faraday Rotation: " << results.faradayRotation_DX_deg << " deg" << std::endl;
    std::cout << "Home Faraday Rotation: " << results.faradayRotation_Home_deg << " deg" << std::endl;
    std::cout << "Total Rotation: " << results.totalRotation_deg << " deg" << std::endl;

    std::cout << "\n--- Ionosphere Parameters (Precise Model) ---" << std::endl;
    const IonosphereData& iono_data = calculator.getIonosphereData();
    std::cout << "DX vTEC: " << std::setprecision(2) << iono_data.vTEC_DX << " TECU" << std::endl;
    std::cout << "DX hmF2: " << iono_data.hmF2_DX << " km" << std::endl;
    std::cout << "DX Mapping Factor: " << std::setprecision(4) << results.slantFactor_DX << std::endl;
    std::cout << "Home vTEC: " << std::setprecision(2) << iono_data.vTEC_Home << " TECU" << std::endl;
    std::cout << "Home hmF2: " << iono_data.hmF2_Home << " km" << std::endl;
    std::cout << "Home Mapping Factor: " << std::setprecision(4) << results.slantFactor_Home << std::endl;

    std::cout << "\n--- Link Parameters ---" << std::endl;
    std::cout << "Path Length: " << std::setprecision(1)
              << results.pathLength_km << " km" << std::endl;
    std::cout << "Propagation Delay: " << std::setprecision(3)
              << results.propagationDelay_ms << " ms" << std::endl;

    std::cout << "\n--- POLARIZATION LOSS ---" << std::endl;
    std::cout << "PLF (Polarization Loss Factor): " << std::setprecision(6)
              << results.PLF << std::endl;
    std::cout << "Loss: " << std::setprecision(3)
              << results.polarizationLoss_dB << " dB" << std::endl;
    std::cout << "Efficiency: " << std::setprecision(2)
              << results.polarizationEfficiency << " %" << std::endl;

    // ========== Interpretation ==========
    std::cout << "\n--- Interpretation ---" << std::endl;
    if (results.polarizationLoss_dB > -1.0) {
        std::cout << "Excellent: Minimal polarization loss." << std::endl;
    } else if (results.polarizationLoss_dB > -3.0) {
        std::cout << "Good: Acceptable polarization loss for most operations." << std::endl;
    } else if (results.polarizationLoss_dB > -6.0) {
        std::cout << "Fair: Moderate loss, may affect weak signal work." << std::endl;
    } else {
        std::cout << "Poor: Significant loss. Consider using circular polarization." << std::endl;
    }

    printSeparator();

    // ========== Save to file option ==========
    std::cout << "\nSave results to file? (y/n): ";
    char save_file;
    std::cin >> save_file;
    clearInputBuffer();

    if (save_file == 'y' || save_file == 'Y') {
        std::cout << "Enter filename (e.g., results.txt): ";
        std::string filename;
        std::getline(std::cin, filename);

        std::ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << std::fixed << std::setprecision(3);
            outfile << "EME Faraday Rotation Calculation Results\n";
            outfile << "=========================================\n\n";
            outfile << "Frequency: " << frequency_MHz << " MHz\n";
            outfile << "DX Grid: " << dx_grid << "\n";
            outfile << "Home Grid: " << home_grid << "\n";
            outfile << "Distance: " << calculator.calculateStationDistance() << " km\n\n";
            outfile << "Total Rotation: " << results.totalRotation_deg << " deg\n";
            outfile << "Polarization Loss: " << results.polarizationLoss_dB << " dB\n";
            outfile << "Efficiency: " << results.polarizationEfficiency << " %\n";
            outfile.close();
            std::cout << "Results saved to " << filename << std::endl;
        } else {
            std::cerr << "Error: Could not open file for writing." << std::endl;
        }
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
