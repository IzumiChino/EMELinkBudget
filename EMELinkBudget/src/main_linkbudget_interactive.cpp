#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include "MoonCalendarReader.h"
#include "AstronomyAPIClient.h"
#include "NOAAGlotecReader.h"
#include "WMMModel.h"
#include "NoiseCalculator.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <ctime>
#include <cmath>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SkyNoiseModel g_skyModel;

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

    site.callsign = getString("Callsign", "");

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

            if (lat < -90.0) lat = -90.0;
            if (lat > 90.0) lat = 90.0;
            if (lon < -180.0) lon = -180.0;
            if (lon > 180.0) lon = 180.0;

            site.latitude = ParameterUtils::deg2rad(lat);
            site.longitude = ParameterUtils::deg2rad(lon);
            site.gridLocator = MaidenheadGrid::latLonToGrid(lat, lon, 6);
        }
    } else {
        double lat = getDouble("Latitude (degrees, -90 to 90)", 0.0);
        double lon = getDouble("Longitude (degrees, -180 to 180)", 0.0);

        if (lat < -90.0) lat = -90.0;
        if (lat > 90.0) lat = 90.0;
        if (lon < -180.0) lon = -180.0;
        if (lon > 180.0) lon = 180.0;

        site.latitude = ParameterUtils::deg2rad(lat);
        site.longitude = ParameterUtils::deg2rad(lon);
        site.gridLocator = MaidenheadGrid::latLonToGrid(lat, lon, 6);
    }

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

    std::cout << "Common frequencies:" << std::endl;
    std::cout << "  50 MHz (6m), 144 MHz (2m), 432 MHz (70cm)" << std::endl;
    std::cout << "  1296 MHz (23cm), 2400 MHz (13cm), 5760 MHz (6cm)" << std::endl;
    params.frequency_MHz = getDouble("Operating frequency (MHz)", 144.0);
    std::cout << "  => Band: " << ParameterUtils::getFrequencyBand(params.frequency_MHz) << std::endl;

    std::cout << "\nTransmitter Configuration:" << std::endl;
    params.txPower_dBm = getDouble("TX Power (dBm, e.g., 50=100W, 40=10W)", 50.0);
    double txPower_W = std::pow(10.0, (params.txPower_dBm - 30.0) / 10.0);
    std::cout << "  => Power: " << std::fixed << std::setprecision(1) << txPower_W << " W" << std::endl;

    params.txGain_dBi = getDouble("TX Antenna Gain (dBi)", 20.0);
    params.txFeedlineLoss_dB = getDouble("TX Feedline Loss (dB)", 0.5);

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

        if (month == 1 && day == 14)
			std::cout << " Happy Birthday Mutsumi Wakaba! " << std::endl;

        std::tm timeinfo = {};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = 0;
        timeinfo.tm_isdst = 0;

        #ifdef _WIN32
        std::time_t obsTime = _mkgmtime(&timeinfo);
        #else
        std::time_t obsTime = timegm(&timeinfo);
        #endif

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC\n", &timeinfo);
        std::cout << "  -> Using: " << timeStr;

        return obsTime;
    } else {
        std::time_t now = std::time(nullptr);

        std::tm* timeinfo = std::gmtime(&now);
        if (timeinfo->tm_mon == 0 && timeinfo->tm_mday == 14)
            std::cout << " Happy Birthday Mutsumi Wakaba! " << std::endl;
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC\n", timeinfo);
        std::cout << "  -> Using current time: " << timeStr;

        return now;
    }
}

void inputMoonEphemeris(MoonEphemeris& moon, std::time_t observationTime, const SiteParameters& txSite) {
    printHeader("Moon Position Data");

    std::cout << "The program needs moon position data for accurate calculations." << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  1. Auto-fetch from Astronomy API (requires internet)" << std::endl;
    std::cout << "  2. Load from moon calendar file (data/calendar.dat)" << std::endl;
    std::cout << "  3. Use estimated position (less accurate)" << std::endl;
    std::cout << "  4. Manual input (if you have data from astronomy software)" << std::endl;

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 1) {
        std::cout << "\nFetching moon position from Astronomy API..." << std::endl;

        AstronomyAPIClient apiClient;
        AstronomyAPIClient::MoonData apiData;

        double txLat_deg = txSite.latitude * 180.0 / M_PI;
        double txLon_deg = txSite.longitude * 180.0 / M_PI;

        std::cout << "[DEBUG] TX Location: " << txLat_deg << "N, " << txLon_deg << "E" << std::endl;
        std::cout << "[DEBUG] Observation time: " << observationTime << std::endl;

        if (apiClient.fetchMoonPosition(observationTime, txLat_deg, txLon_deg, apiData)) {
            moon.rightAscension = apiData.ra_deg * M_PI / 180.0;
            moon.declination = apiData.dec_deg * M_PI / 180.0;
            moon.distance_km = apiData.distance_km;
            moon.rangeRate_km_s = apiData.range_rate_km_s;
            moon.librationLon_deg = apiData.libration_lon_deg;
            moon.librationLat_deg = apiData.libration_lat_deg;
            moon.librationLonRate_deg_day = apiData.libration_lon_rate_deg_day;
            moon.librationLatRate_deg_day = apiData.libration_lat_rate_deg_day;

            if (moon.librationLonRate_deg_day == 0.0 && moon.librationLatRate_deg_day == 0.0) {
                double lunarMonth_days = 27.32166;
                double librationLonAmplitude_deg = 7.9;
                double librationLatAmplitude_deg = 6.7;
                moon.librationLonRate_deg_day = (2.0 * M_PI * librationLonAmplitude_deg) / lunarMonth_days;
                moon.librationLatRate_deg_day = (2.0 * M_PI * librationLatAmplitude_deg) / lunarMonth_days;
            }

            moon.hourAngle_DX = 0.0;
            moon.hourAngle_Home = 0.0;
            moon.ephemerisSource = "JPL Horizons";

            std::cout << "[OK] Moon position fetched successfully!" << std::endl;
            std::cout << "  => RA: " << std::fixed << std::setprecision(2)
                      << apiData.ra_deg << " deg" << std::endl;
            std::cout << "  => DEC: " << apiData.dec_deg << " deg" << std::endl;
            std::cout << "  => Distance: " << std::setprecision(1)
                      << apiData.distance_km << " km" << std::endl;

            if (apiData.libration_lon_rate_deg_day != 0.0 || apiData.libration_lat_rate_deg_day != 0.0) {
                std::cout << "  => Libration rates: Lon=" << std::setprecision(3)
                          << apiData.libration_lon_rate_deg_day << " deg/day, Lat="
                          << apiData.libration_lat_rate_deg_day << " deg/day" << std::endl;
            } else {
                std::cout << "  => Using estimated libration rates" << std::endl;
            }

            // Try to improve DEC accuracy with calendar data
            MoonCalendarReader calendar;
            std::tm* timeInfo = std::gmtime(&observationTime);
            double dec_calendar;
            if (calendar.loadCalendarFile("data/calendar.dat") &&
                calendar.getMoonDeclination(*timeInfo, dec_calendar)) {
                moon.declination = dec_calendar * M_PI / 180.0;
                std::cout << "  => DEC refined: " << dec_calendar
                          << " deg (from calendar interpolation)" << std::endl;
            }

            return;
        } else {
            std::cout << "[!] API fetch failed: " << apiClient.getLastError() << std::endl;
            std::cout << "Falling back to moon calendar file...\n" << std::endl;
            choice = 2;
        }
    }

    if (choice == 2) {
        MoonCalendarReader calendar;
        if (calendar.loadCalendarFile("data/calendar.dat")) {
            std::cout << "Loading moon position from calendar file..." << std::endl;

            std::tm* timeInfo = std::localtime(&observationTime);

            double declination;
            if (calendar.getMoonDeclination(*timeInfo, declination)) {
                moon.declination = declination * M_PI / 180.0;

                int dayOfYear = timeInfo->tm_yday;
                double estimatedRA = std::fmod(180.0 + dayOfYear * 13.2, 360.0);
                moon.rightAscension = estimatedRA * M_PI / 180.0;

                moon.distance_km = 384400.0;
                moon.hourAngle_DX = 0.0;
                moon.hourAngle_Home = 0.0;
                moon.ephemerisSource = "Moon Calendar";

                std::cout << "  => RA: " << std::fixed << std::setprecision(1)
                          << estimatedRA << " deg (estimated from date)" << std::endl;
                std::cout << "  => DEC: " << declination << " deg (from calendar)" << std::endl;
                std::cout << "  => Distance: 384400 km (average)" << std::endl;
                std::cout << "[OK] Moon calendar loaded successfully" << std::endl;
                return;
            } else {
                std::cout << "[!] Could not find moon data for this date in calendar." << std::endl;
                std::cout << "Falling back to estimated position...\n" << std::endl;
                choice = 3;
            }
        } else {
            std::cout << "[!] Could not load calendar file: data/calendar.dat" << std::endl;
            std::cout << "Falling back to estimated position...\n" << std::endl;
            choice = 3;
        }
    }

    if (choice == 3) {
        std::cout << "Using estimated moon position (approximate)..." << std::endl;
        std::cout << "[!] Note: For accurate results, use real ephemeris data!" << std::endl;

        moon.rightAscension = 180.0 * M_PI / 180.0;
        moon.declination = 15.0 * M_PI / 180.0;
        moon.distance_km = 384400.0;
        moon.hourAngle_DX = 0.0;
        moon.hourAngle_Home = 0.0;
        moon.ephemerisSource = "Estimated";

        std::cout << "  => RA: 180.0 deg (estimated)" << std::endl;
        std::cout << "  => DEC: 15.0 deg (estimated)" << std::endl;
        std::cout << "  => Distance: 384400 km (average)" << std::endl;
    } else if (choice == 4) {
        std::cout << "\nIf you have astronomy software (Stellarium, WSJT-X, etc.)," << std::endl;
        std::cout << "you can get accurate moon position data:\n" << std::endl;

        double ra = getDouble("Right Ascension (degrees, 0-360)", 180.0);
        double dec = getDouble("Declination (degrees, -90 to 90)", 15.0);
        double dist = getDouble("Distance (km, typical: 356000-406000)", 384400.0);

        moon.rightAscension = ra * M_PI / 180.0;
        moon.declination = dec * M_PI / 180.0;
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
        std::cout << "\nAttempting to fetch real-time ionosphere data..." << std::endl;

        std::tm* timeInfo = std::gmtime(&observationTime);

        NOAAGlotecReader glotecReader;
        GlotecData glotecData;

        std::string url = glotecReader.getDataUrl(*timeInfo);
        std::cout << "[DEBUG] GLOTEC URL: " << url << std::endl;

        bool glotecSuccess = glotecReader.fetchTecData(*timeInfo, glotecData);

        if (!glotecSuccess) {
            std::cout << "[DEBUG] GLOTEC fetch failed - check network connection or data availability" << std::endl;
        } else {
            std::cout << "[DEBUG] GLOTEC data fetched, grid size: "
                      << glotecData.numLon << "x" << glotecData.numLat << std::endl;
        }

        if (glotecSuccess) {
            double lat_tx = ParameterUtils::rad2deg(txSite.latitude);
            double lon_tx = ParameterUtils::rad2deg(txSite.longitude);
            double lat_rx = ParameterUtils::rad2deg(rxSite.latitude);
            double lon_rx = ParameterUtils::rad2deg(rxSite.longitude);

            double tec_tx = 0.0, tec_rx = 0.0;
            bool tx_ok = glotecReader.getTecAtLocation(glotecData, lat_tx, lon_tx, tec_tx);
            bool rx_ok = glotecReader.getTecAtLocation(glotecData, lat_rx, lon_rx, tec_rx);

            if (tx_ok && rx_ok) {
                iono.vTEC_DX = tec_tx;
                iono.vTEC_Home = tec_rx;

                std::cout << "[OK] GLOTEC TEC data fetched successfully" << std::endl;
                std::cout << "  TX TEC: " << std::fixed << std::setprecision(1) << tec_tx << " TECU" << std::endl;
                std::cout << "  RX TEC: " << tec_rx << " TECU" << std::endl;

                WMMModel wmm;
                const char* wmmPaths[] = {
                    "data/WMMHR.COF",
                    "EMELinkBudget/data/WMMHR.COF",
                    "../data/WMMHR.COF",
                    "../EMELinkBudget/data/WMMHR.COF"
                };

                bool wmmLoaded = false;
                for (const char* path : wmmPaths) {
                    if (wmm.loadCoefficientFile(path)) {
                        wmmLoaded = true;
                        break;
                    }
                }

                if (wmmLoaded) {
                    int year = timeInfo->tm_year + 1900;
                    int month = timeInfo->tm_mon + 1;
                    int day = timeInfo->tm_mday;
                    int hour = timeInfo->tm_hour;
                    int minute = timeInfo->tm_min;

                    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                    int days_in_year = is_leap ? 366 : 365;
                    int days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
                    int day_of_year = days_before_month[month - 1] + day;
                    if (is_leap && month > 2) day_of_year += 1;

                    double decimal_year = year + (day_of_year - 1 + hour / 24.0 + minute / 1440.0) / days_in_year;

                    double height_tx_km = 0.0;
                    double height_rx_km = 0.0;

                    MagneticFieldResult mag_tx = wmm.calculate(lat_tx, lon_tx, height_tx_km, decimal_year);
                    MagneticFieldResult mag_rx = wmm.calculate(lat_rx, lon_rx, height_rx_km, decimal_year);

                    iono.B_magnitude_DX = mag_tx.F * 1e-9;
                    iono.B_magnitude_Home = mag_rx.F * 1e-9;
                    iono.B_inclination_DX = mag_tx.inclination * M_PI / 180.0;
                    iono.B_inclination_Home = mag_rx.inclination * M_PI / 180.0;
                    iono.B_declination_DX = mag_tx.declination * M_PI / 180.0;
                    iono.B_declination_Home = mag_rx.declination * M_PI / 180.0;

                    iono.hmF2_DX = 350.0;
                    iono.hmF2_Home = 350.0;

                    iono.dataSource = "GLOTEC + WMM";

                    std::cout << "[OK] WMM magnetic field data loaded" << std::endl;
                    std::cout << "  TX Magnetic inclination: " << std::fixed << std::setprecision(1)
                              << mag_tx.inclination << " deg" << std::endl;
                    std::cout << "  RX Magnetic inclination: " << mag_rx.inclination << " deg" << std::endl;

                    return;
                } else {
                    std::cout << "[!] Could not load WMM model (tried multiple paths)" << std::endl;
                    std::cout << "Using estimated magnetic field values..." << std::endl;

                    iono.B_magnitude_DX = 5.0e-5;
                    iono.B_magnitude_Home = 5.0e-5;

                    double incl_tx = std::abs(lat_tx) * 1.2;
                    double incl_rx = std::abs(lat_rx) * 1.2;
                    if (incl_tx > 90.0) incl_tx = 90.0;
                    if (incl_rx > 90.0) incl_rx = 90.0;

                    iono.B_inclination_DX = ParameterUtils::deg2rad(incl_tx);
                    iono.B_inclination_Home = ParameterUtils::deg2rad(incl_rx);
                    iono.B_declination_DX = 0.0;
                    iono.B_declination_Home = 0.0;

                    iono.hmF2_DX = 350.0;
                    iono.hmF2_Home = 350.0;

                    iono.dataSource = "GLOTEC + Estimated Magnetic";

                    std::cout << "  TX Magnetic inclination: " << std::fixed << std::setprecision(1)
                              << incl_tx << " deg (estimated)" << std::endl;
                    std::cout << "  RX Magnetic inclination: " << incl_rx << " deg (estimated)" << std::endl;

                    return;
                }
            }
        }

        std::cout << "[!] Failed to fetch GLOTEC data" << std::endl;
        std::cout << "Falling back to typical values...\n" << std::endl;
        choice = 2;
    }

    if (choice == 2) {
        std::cout << "Using typical ionosphere values..." << std::endl;
        std::cout << "[!] Note: Actual values vary by location, time, and solar activity!" << std::endl;

        double lat_tx = ParameterUtils::rad2deg(txSite.latitude);
        double lat_rx = ParameterUtils::rad2deg(rxSite.latitude);

        iono.vTEC_DX = 25.0;
        iono.vTEC_Home = 25.0;

        iono.hmF2_DX = 350.0;
        iono.hmF2_Home = 350.0;

        iono.B_magnitude_DX = 5.0e-5;
        iono.B_magnitude_Home = 5.0e-5;

        double incl_tx = std::abs(lat_tx) * 1.2;
        double incl_rx = std::abs(lat_rx) * 1.2;
        if (incl_tx > 90.0) incl_tx = 90.0;
        if (incl_rx > 90.0) incl_rx = 90.0;

        iono.B_inclination_DX = ParameterUtils::deg2rad(incl_tx);
        iono.B_inclination_Home = ParameterUtils::deg2rad(incl_rx);

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

    std::cout << "\n[*] Geometry & Moon Position:" << std::endl;
    std::cout << "  Moon RA/DEC: " << std::fixed << std::setprecision(2)
              << results.geometry.moonRA_deg << " deg / "
              << results.geometry.moonDEC_deg << " deg" << std::endl;
    std::cout << "  Moon Distance: " << results.geometry.moonDistance_km << " km" << std::endl;
    std::cout << "  TX Elevation: " << results.geometry.moonElevation_TX_deg << " deg" << std::endl;
    std::cout << "  RX Elevation: " << results.geometry.moonElevation_RX_deg << " deg" << std::endl;
    std::cout << "  Path Length: " << results.geometry.totalPathLength_km << " km" << std::endl;

    if (results.geometry.spectralSpread_Hz > 0.0) {
        std::cout << "\n[*] Spectral Spreading (Libration Effects):" << std::endl;
        std::cout << "  Doppler Spread: " << std::setprecision(3)
                  << results.geometry.spectralSpread_Hz << " Hz" << std::endl;
        std::cout << "  Coherent Integration Limit: " << std::setprecision(3)
                  << results.geometry.coherentIntegrationLimit_s << " s" << std::endl;
        std::cout << "  Libration Velocity: " << std::setprecision(2)
                  << results.geometry.librationVelocity_m_s << " m/s" << std::endl;
    }

    std::cout << "\n[*] Path Loss Analysis:" << std::endl;
    std::cout << "  Free Space Loss: " << results.pathLoss.freeSpaceLoss_dB << " dB" << std::endl;
    std::cout << "  Lunar Scattering: " << results.pathLoss.lunarScatteringLoss_dB << " dB";
    if (results.pathLoss.useHagforsModel) {
        std::cout << " (Hagfors' Law)" << std::endl;
        std::cout << "    - Bistatic Angle: " << std::setprecision(2)
                  << results.pathLoss.bistaticAngle_deg << " deg" << std::endl;
        std::cout << "    - Roughness Param: " << std::setprecision(3)
                  << results.pathLoss.hagforsRoughnessParam << std::endl;
        std::cout << "    - Lunar RCS: " << std::setprecision(2)
                  << results.pathLoss.lunarRCS_dBsm << " dBsm" << std::endl;
    } else {
        std::cout << " (Simple Model)" << std::endl;
    }
    std::cout << "  Atmospheric Loss: " << results.pathLoss.atmosphericLoss_Total_dB << " dB" << std::endl;
    std::cout << "  Total Path Loss: " << results.pathLoss.totalPathLoss_dB << " dB" << std::endl;

    std::cout << "\n[*] Polarization Analysis:" << std::endl;
    std::cout << "  Spatial Rotation: " << std::setprecision(3)
              << results.polarization.spatialRotation_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (TX): " << results.polarization.faradayRotation_TX_deg << " deg" << std::endl;
    std::cout << "  Faraday Rotation (RX): " << results.polarization.faradayRotation_RX_deg << " deg" << std::endl;
    std::cout << "  Total Rotation: " << results.polarization.totalRotation_deg << " deg" << std::endl;
    std::cout << "  Polarization Loss: " << std::setprecision(2)
              << results.polarization.polarizationLoss_dB << " dB" << std::endl;
    std::cout << "  PLF: " << std::setprecision(6) << results.polarization.PLF << std::endl;

    std::cout << "\n[*] Noise Analysis:" << std::endl;
    std::cout << "  Sky Noise: " << std::setprecision(1)
              << results.noise.skyNoiseTemp_K << " K";

    if (g_skyModel.isMapLoaded()) {
        std::cout << " (Haslam 408 MHz map)" << std::endl;
    } else {
        std::cout << " (Simplified model)" << std::endl;
    }

    std::cout << "  Ground Spillover: " << results.noise.groundSpilloverTemp_K << " K" << std::endl;
    std::cout << "  System Noise: " << results.noise.systemNoiseTemp_K << " K" << std::endl;
    std::cout << "  Noise Power: " << std::setprecision(2)
              << results.noise.noisePower_dBm << " dBm" << std::endl;

    std::cout << "\n[*] Signal-to-Noise Ratio:" << std::endl;
    std::cout << "  Received Power: " << results.snr.receivedSignalPower_dBm << " dBm" << std::endl;
    std::cout << "  SNR: " << results.snr.SNR_dB << " dB" << std::endl;
    std::cout << "  Fading Margin: " << results.snr.fadingMargin_dB << " dB" << std::endl;
    std::cout << "  Effective SNR: " << results.snr.effectiveSNR_dB << " dB" << std::endl;
    std::cout << "  Required SNR: " << results.snr.requiredSNR_dB << " dB (Q65 + AP decode)" << std::endl;

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

    std::cout << "[*] Loading Haslam 408 MHz Sky Map..." << std::endl;

    const char* haslamPaths[] = {
        "EMELinkBudget/data/haslam408_dsds_Remazeilles2014_ns2048.fits",
        "data/haslam408_dsds_Remazeilles2014_ns2048.fits",
        "../EMELinkBudget/data/haslam408_dsds_Remazeilles2014_ns2048.fits",
        "../../EMELinkBudget/data/haslam408_dsds_Remazeilles2014_ns2048.fits"
    };

    bool haslamLoaded = false;
    for (const char* path : haslamPaths) {
        if (g_skyModel.loadSkyMap(path)) {
            haslamLoaded = true;
            std::cout << "[+] Haslam sky map loaded successfully" << std::endl;
            break;
        }
    }

    if (!haslamLoaded) {
        std::cout << "[!] Could not load Haslam sky map, using simplified model" << std::endl;
    }
    std::cout << std::endl;

    LinkBudgetParameters params;

    params.observationTime = inputObservationTime();

    inputStationData("TX (DX)", params.txSite);

    inputStationData("RX (Home)", params.rxSite);

    inputSystemConfiguration(params);

    inputMoonEphemeris(params.moonEphemeris, params.observationTime, params.txSite);

    inputIonosphereData(params.ionosphereData, params.observationTime,
                        params.txSite, params.rxSite);

    printHeader("Calculation Options");
    std::cout << "Enable advanced physical effects (recommended: all yes):\n" << std::endl;
    params.includeFaradayRotation = getYesNo("Include Faraday rotation (ionosphere effect)");
    params.includeSpatialRotation = getYesNo("Include spatial rotation (geometry effect)");
    params.includeMoonReflection = getYesNo("Include moon reflection (polarization flip)");
    params.includeAtmosphericLoss = getYesNo("Include atmospheric loss");
    params.includeGroundSpillover = getYesNo("Include ground spillover noise");
    params.useHagforsModel = getYesNo("Use Hagfors' Law for lunar scattering (recommended)");
    std::cout << std::endl;

    std::cout << "Calculating link budget..." << std::endl;
    EMELinkBudget linkBudget(params);
    LinkBudgetResults results = linkBudget.calculate();

    displayResults(results);

    std::cout << "\n";
    if (getYesNo("Calculate another link")) {
        std::cout << "\n\n";
        return main();
    }

    std::cout << "\nThank you for using EME Link Budget Calculator!" << std::endl;
    std::cout << "\nTips for better accuracy:" << std::endl;
    std::cout << "  * Use real moon position from JPL Horizons or WSJT-X" << std::endl;
    std::cout << "  * Use real-time TEC data from IONEX files" << std::endl;
    std::cout << "  * Measure your actual system parameters" << std::endl;
    std::cout << "  * Check results during actual EME QSOs" << std::endl;

    return 0;
}
