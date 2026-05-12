#define _CRT_SECURE_NO_WARNINGS

#include "EMELinkBudget.h"
#include "MaidenheadGrid.h"
#include "MoonCalendarReader.h"
#include "AstronomyAPIClient.h"
#include "NOAAGlotecReader.h"
#include "WMMModel.h"
#include "NoiseCalculator.h"
#include "EnterpriseConfig.h"
#include "DebugUtils.h"
#include "TimeUtils.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <ctime>
#include <cmath>
#include <numbers>
#include <filesystem>
#include <exception>
#include <unistd.h>

SkyNoiseModel g_skyModel;

namespace {

constexpr double kPi = std::numbers::pi_v<double>;
constexpr double kDegreesToRadians = kPi / 180.0;
constexpr double kRadiansToDegrees = 180.0 / kPi;

std::vector<std::string> buildWmmSearchPaths() {
    const std::vector<std::filesystem::path> relativeCandidates = {
        "data/WMMHR.COF",
        "EMELinkBudget/data/WMMHR.COF",
        "build/data/WMMHR.COF",
        "build/release/EMELinkBudget/data/WMMHR.COF",
        "build/debug/EMELinkBudget/data/WMMHR.COF"
    };

    std::vector<std::filesystem::path> basePaths;
    std::error_code ec;

    auto current = std::filesystem::current_path(ec);
    if (!ec) {
        basePaths.push_back(current);
        auto cursor = current;
        for (int i = 0; i < 6 && cursor.has_parent_path(); ++i) {
            cursor = cursor.parent_path();
            basePaths.push_back(cursor);
        }
    }

    ec.clear();
    auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        auto exeDir = exePath.parent_path();
        basePaths.push_back(exeDir);
        auto cursor = exeDir;
        for (int i = 0; i < 6 && cursor.has_parent_path(); ++i) {
            cursor = cursor.parent_path();
            basePaths.push_back(cursor);
        }
    }

    const std::vector<std::string> legacyPaths = {
        "data/WMMHR.COF",
        "EMELinkBudget/data/WMMHR.COF",
        "../data/WMMHR.COF",
        "../EMELinkBudget/data/WMMHR.COF",
        "../../data/WMMHR.COF",
        "../../EMELinkBudget/data/WMMHR.COF"
    };

    std::vector<std::string> paths;
    paths.reserve(basePaths.size() * relativeCandidates.size() + legacyPaths.size());

    for (const auto& base : basePaths) {
        for (const auto& rel : relativeCandidates) {
            paths.push_back((base / rel).lexically_normal().string());
        }
    }
    paths.insert(paths.end(), legacyPaths.begin(), legacyPaths.end());

    return paths;
}

bool tryLoadWmmModel(WMMModel& wmm, std::string& loadedPath) {
    const auto candidates = buildWmmSearchPaths();
    for (const auto& path : candidates) {
        if (wmm.loadCoefficientFile(path)) {
            loadedPath = path;
            return true;
        }
    }
    return false;
}

}  // namespace

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printSeparator(char c = '=', int length = 80) {
    std::cout << std::string(length, c) << '\n';
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "  " << title << '\n';
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
    } catch (const std::exception&) {
        std::cout << "Invalid input, using default: " << defaultValue << '\n';
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

    site.callsign = getString("Callsign", site.callsign);

    std::string grid = getString("Maidenhead Grid Locator (e.g., OM81ks)", site.gridLocator);

    if (!grid.empty()) {
        try {
            double lat, lon;
            MaidenheadGrid::gridToLatLon(grid, lat, lon);
            site.latitude = ParameterUtils::deg2rad(lat);
            site.longitude = ParameterUtils::deg2rad(lon);
            site.gridLocator = grid;
            std::cout << "  => Latitude: " << std::fixed << std::setprecision(4)
                      << lat << " deg" << '\n';
            std::cout << "  => Longitude: " << lon << " deg" << '\n';
        } catch (const std::exception&) {
            std::cout << "Invalid grid locator, using manual input..." << '\n';
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

    std::cout << "\nPolarization Configuration:" << '\n';
    std::cout << "  1. Linear Horizontal (psi=0, chi=0)" << '\n';
    std::cout << "  2. Linear Vertical (psi=90, chi=0)" << '\n';
    std::cout << "  3. RHCP (psi=0, chi=45)" << '\n';
    std::cout << "  4. LHCP (psi=0, chi=-45)" << '\n';
    std::cout << "  5. Custom" << '\n';

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

    std::cout << "  => Polarization: " << ParameterUtils::getPolarizationType(site.chi) << '\n';
    std::cout << '\n';
}

void inputSystemConfiguration(LinkBudgetParameters& params) {
    printHeader("System Configuration");

    std::cout << "Common frequencies:" << '\n';
    std::cout << "  50 MHz (6m), 144 MHz (2m), 432 MHz (70cm)" << '\n';
    std::cout << "  1296 MHz (23cm), 2400 MHz (13cm), 5760 MHz (6cm)" << '\n';
    params.frequency_MHz = getDouble("Operating frequency (MHz)", params.frequency_MHz);
    std::cout << "  => Band: " << ParameterUtils::getFrequencyBand(params.frequency_MHz) << '\n';

    std::cout << "\nTransmitter Configuration:" << '\n';
    params.txPower_dBm = getDouble("TX Power (dBm, e.g., 50=100W, 40=10W)", params.txPower_dBm);
    double txPower_W = std::pow(10.0, (params.txPower_dBm - 30.0) / 10.0);
    std::cout << "  => Power: " << std::fixed << std::setprecision(1) << txPower_W << " W" << '\n';

    params.txGain_dBi = getDouble("TX Antenna Gain (dBi)", params.txGain_dBi);
    params.txFeedlineLoss_dB = getDouble("TX Feedline Loss (dB)", params.txFeedlineLoss_dB);

    std::cout << "\nReceiver Configuration:" << '\n';
    params.rxGain_dBi = getDouble("RX Antenna Gain (dBi)", params.rxGain_dBi);
    params.rxFeedlineLoss_dB = getDouble("RX Feedline Loss (dB)", params.rxFeedlineLoss_dB);
    params.rxNoiseFigure_dB = getDouble("RX Noise Figure (dB, typical LNA: 0.3-0.8)", params.rxNoiseFigure_dB);
    params.bandwidth_Hz = getDouble("Bandwidth (Hz, WSJT-X: 3200)", params.bandwidth_Hz);

    std::cout << '\n';
}

std::time_t inputObservationTime() {
    printHeader("Observation Time");

    std::cout << "When do you want to calculate the link budget?" << '\n';
    std::cout << "  1. Current time (now)" << '\n';
    std::cout << "  2. Specify date and time" << '\n';

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 2) {
        std::cout << "\nEnter observation time (UTC):" << '\n';
        int year = static_cast<int>(getDouble("  Year", 2026.0));
        int month = static_cast<int>(getDouble("  Month (1-12)", 2.0));
        int day = static_cast<int>(getDouble("  Day (1-31)", 16.0));
        int hour = static_cast<int>(getDouble("  Hour (0-23)", 12.0));
        int minute = static_cast<int>(getDouble("  Minute (0-59)", 0.0));

        if (month == 1 && day == 14)
			std::cout << " Happy Birthday Mutsumi Wakaba! " << '\n';

        std::tm timeinfo = {};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = 0;
        timeinfo.tm_isdst = 0;

        std::time_t obsTime = eme::time_utils::toUtcTime(timeinfo);

        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC\n", &timeinfo);
        std::cout << "  -> Using: " << timeStr;

        return obsTime;
    } else {
        std::time_t now = std::time(nullptr);

        std::tm timeinfo = {};
        if (!eme::time_utils::toUtcTm(now, timeinfo)) {
            return now;
        }

        if (timeinfo.tm_mon == 0 && timeinfo.tm_mday == 14)
            std::cout << " Happy Birthday Mutsumi Wakaba! " << '\n';
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S UTC\n", &timeinfo);
        std::cout << "  -> Using current time: " << timeStr;

        return now;
    }
}

void inputMoonEphemeris(MoonEphemeris& moon, std::time_t observationTime, const SiteParameters& txSite) {
    printHeader("Moon Position Data");

    std::cout << "The program needs moon position data for accurate calculations." << '\n';
    std::cout << "\nOptions:" << '\n';
    std::cout << "  1. Auto-fetch from Astronomy API (requires internet)" << '\n';
    std::cout << "  2. Load from moon calendar file (data/calendar.dat)" << '\n';
    std::cout << "  3. Use estimated position (less accurate)" << '\n';
    std::cout << "  4. Manual input (if you have data from astronomy software)" << '\n';

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 1) {
        std::cout << "\nFetching moon position from Astronomy API..." << '\n';

        AstronomyAPIClient apiClient;
        AstronomyAPIClient::MoonData apiData;

        double txLat_deg = txSite.latitude * kRadiansToDegrees;
        double txLon_deg = txSite.longitude * kRadiansToDegrees;

        if (eme::debug::isEnabled()) {
            std::cout << "[DEBUG] TX Location: " << txLat_deg << "N, " << txLon_deg << "E" << '\n';
            std::cout << "[DEBUG] Observation time: " << observationTime << '\n';
        }

        if (apiClient.fetchMoonPosition(observationTime, txLat_deg, txLon_deg, apiData)) {
            moon.rightAscension = apiData.ra_deg * kDegreesToRadians;
            moon.declination = apiData.dec_deg * kDegreesToRadians;
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
                moon.librationLonRate_deg_day = (2.0 * kPi * librationLonAmplitude_deg) / lunarMonth_days;
                moon.librationLatRate_deg_day = (2.0 * kPi * librationLatAmplitude_deg) / lunarMonth_days;
            }

            moon.hourAngle_DX = 0.0;
            moon.hourAngle_Home = 0.0;
            moon.ephemerisSource = "JPL Horizons";

            std::cout << "[OK] Moon position fetched successfully!" << '\n';
            std::cout << "  => RA: " << std::fixed << std::setprecision(2)
                      << apiData.ra_deg << " deg" << '\n';
            std::cout << "  => DEC: " << apiData.dec_deg << " deg" << '\n';
            std::cout << "  => Distance: " << std::setprecision(1)
                      << apiData.distance_km << " km" << '\n';

            if (apiData.libration_lon_rate_deg_day != 0.0 || apiData.libration_lat_rate_deg_day != 0.0) {
                std::cout << "  => Libration rates: Lon=" << std::setprecision(3)
                          << apiData.libration_lon_rate_deg_day << " deg/day, Lat="
                          << apiData.libration_lat_rate_deg_day << " deg/day" << '\n';
            } else {
                std::cout << "  => Using estimated libration rates" << '\n';
            }

            // Try to improve DEC accuracy with calendar data
            MoonCalendarReader calendar;
            std::tm timeInfo = {};
            double dec_calendar;
            if (eme::time_utils::toUtcTm(observationTime, timeInfo) &&
                calendar.loadCalendarFile("data/calendar.dat", timeInfo.tm_year + 1900) &&
                calendar.getMoonDeclination(timeInfo, dec_calendar)) {
                moon.declination = dec_calendar * kDegreesToRadians;
                std::cout << "  => DEC refined: " << dec_calendar
                          << " deg (from calendar interpolation)" << '\n';
            }

            return;
        } else {
            std::cout << "[!] API fetch failed: " << apiClient.getLastError() << '\n';
            std::cout << "Falling back to moon calendar file...\n" << '\n';
            choice = 2;
        }
    }

    if (choice == 2) {
        MoonCalendarReader calendar;
        std::tm timeInfo = {};
        if (!eme::time_utils::toUtcTm(observationTime, timeInfo)) {
            std::cout << "[!] Could not convert observation time to UTC." << '\n';
            std::cout << "Falling back to estimated position...\n" << '\n';
            choice = 3;
        }

        if (choice == 2 && calendar.loadCalendarFile("data/calendar.dat", timeInfo.tm_year + 1900)) {
            std::cout << "Loading moon position from calendar file..." << '\n';

            double declination;
            if (calendar.getMoonDeclination(timeInfo, declination)) {
                moon.declination = declination * kDegreesToRadians;

                int dayOfYear = timeInfo.tm_yday;
                double estimatedRA = std::fmod(180.0 + dayOfYear * 13.2, 360.0);
                moon.rightAscension = estimatedRA * kDegreesToRadians;

                moon.distance_km = 384400.0;
                moon.hourAngle_DX = 0.0;
                moon.hourAngle_Home = 0.0;
                moon.ephemerisSource = "Moon Calendar";

                std::cout << "  => RA: " << std::fixed << std::setprecision(1)
                          << estimatedRA << " deg (estimated from date)" << '\n';
                std::cout << "  => DEC: " << declination << " deg (from calendar)" << '\n';
                std::cout << "  => Distance: 384400 km (average)" << '\n';
                std::cout << "[OK] Moon calendar loaded successfully" << '\n';
                return;
            } else {
                std::cout << "[!] Could not find moon data for this date in calendar." << '\n';
                std::cout << "Falling back to estimated position...\n" << '\n';
                choice = 3;
            }
        } else {
            std::cout << "[!] Could not load calendar file: data/calendar.dat" << '\n';
            std::cout << "Falling back to estimated position...\n" << '\n';
            choice = 3;
        }
    }

    if (choice == 3) {
        std::cout << "Using estimated moon position (approximate)..." << '\n';
        std::cout << "[!] Note: For accurate results, use real ephemeris data!" << '\n';

        moon.rightAscension = 180.0 * kDegreesToRadians;
        moon.declination = 15.0 * kDegreesToRadians;
        moon.distance_km = 384400.0;
        moon.hourAngle_DX = 0.0;
        moon.hourAngle_Home = 0.0;
        moon.ephemerisSource = "Estimated";

        std::cout << "  => RA: 180.0 deg (estimated)" << '\n';
        std::cout << "  => DEC: 15.0 deg (estimated)" << '\n';
        std::cout << "  => Distance: 384400 km (average)" << '\n';
    } else if (choice == 4) {
        std::cout << "\nIf you have astronomy software (Stellarium, WSJT-X, etc.)," << '\n';
        std::cout << "you can get accurate moon position data:\n" << '\n';

        double ra = getDouble("Right Ascension (degrees, 0-360)", 180.0);
        double dec = getDouble("Declination (degrees, -90 to 90)", 15.0);
        double dist = getDouble("Distance (km, typical: 356000-406000)", 384400.0);

        moon.rightAscension = ra * kDegreesToRadians;
        moon.declination = dec * kDegreesToRadians;
        moon.distance_km = dist;
        moon.hourAngle_DX = 0.0;
        moon.hourAngle_Home = 0.0;
        moon.ephemerisSource = "Manual Input";
    }

    std::cout << '\n';
}

void inputIonosphereData(IonosphereData& iono, std::time_t observationTime,
                         const SiteParameters& txSite, const SiteParameters& rxSite) {
    printHeader("Ionosphere Data");

    std::cout << "The program needs ionosphere data (TEC and magnetic field) for" << '\n';
    std::cout << "accurate Faraday rotation calculations." << '\n';
    std::cout << "\nOptions:" << '\n';
    std::cout << "  1. Auto-fetch from IONEX/GLOTEC (requires internet)" << '\n';
    std::cout << "  2. Use typical values (less accurate)" << '\n';
    std::cout << "  3. Manual input (if you have measured data)" << '\n';

    int choice = static_cast<int>(getDouble("Select option", 1.0));

    if (choice == 1) {
        std::cout << "\nAttempting to fetch real-time ionosphere data..." << '\n';

        std::tm timeInfo = {};
        if (!eme::time_utils::toUtcTm(observationTime, timeInfo)) {
            std::cout << "[!] Could not convert observation time to UTC; using fallback values." << '\n';
            choice = 2;
        }

        NOAAGlotecReader glotecReader;
        GlotecData glotecData;

        std::string url = glotecReader.getDataUrl(timeInfo);
        if (eme::debug::isEnabled()) {
            std::cout << "[DEBUG] GLOTEC URL: " << url << '\n';
        }

        bool glotecSuccess = (choice == 1) && glotecReader.fetchTecData(timeInfo, glotecData);

        if (eme::debug::isEnabled()) {
            if (!glotecSuccess) {
                std::cout << "[DEBUG] GLOTEC fetch failed - check network connection or data availability" << '\n';
            } else {
                std::cout << "[DEBUG] GLOTEC data fetched, grid size: "
                          << glotecData.numLon << "x" << glotecData.numLat << '\n';
            }
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

                std::cout << "[OK] GLOTEC TEC data fetched successfully" << '\n';
                std::cout << "  TX TEC: " << std::fixed << std::setprecision(1) << tec_tx << " TECU" << '\n';
                std::cout << "  RX TEC: " << tec_rx << " TECU" << '\n';

                WMMModel wmm;
                std::string loadedWmmPath;
                bool wmmLoaded = tryLoadWmmModel(wmm, loadedWmmPath);

                if (wmmLoaded) {
                    std::cout << "[OK] WMM model loaded from: " << loadedWmmPath << '\n';

                    int year = timeInfo.tm_year + 1900;
                    int month = timeInfo.tm_mon + 1;
                    int day = timeInfo.tm_mday;
                    int hour = timeInfo.tm_hour;
                    int minute = timeInfo.tm_min;

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
                    iono.B_inclination_DX = mag_tx.inclination * kDegreesToRadians;
                    iono.B_inclination_Home = mag_rx.inclination * kDegreesToRadians;
                    iono.B_declination_DX = mag_tx.declination * kDegreesToRadians;
                    iono.B_declination_Home = mag_rx.declination * kDegreesToRadians;

                    iono.hmF2_DX = 350.0;
                    iono.hmF2_Home = 350.0;

                    iono.dataSource = "GLOTEC + WMM";

                    std::cout << "[OK] WMM magnetic field data loaded" << '\n';
                    std::cout << "  TX Magnetic inclination: " << std::fixed << std::setprecision(1)
                              << mag_tx.inclination << " deg" << '\n';
                    std::cout << "  RX Magnetic inclination: " << mag_rx.inclination << " deg" << '\n';

                    return;
                } else {
                    std::cout << "[!] Could not load WMM model (tried multiple paths)" << '\n';
                    std::cout << "Using estimated magnetic field values..." << '\n';

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
                              << incl_tx << " deg (estimated)" << '\n';
                    std::cout << "  RX Magnetic inclination: " << incl_rx << " deg (estimated)" << '\n';

                    return;
                }
            }
        }

        std::cout << "[!] Failed to fetch GLOTEC data" << '\n';
        std::cout << "Falling back to typical values...\n" << '\n';
        choice = 2;
    }

    if (choice == 2) {
        std::cout << "Using typical ionosphere values..." << '\n';
        std::cout << "[!] Note: Actual values vary by location, time, and solar activity!" << '\n';

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

        std::cout << "  TX Station:" << '\n';
        std::cout << "    => TEC: " << iono.vTEC_DX << " TECU (typical)" << '\n';
        std::cout << "    => Magnetic inclination: " << std::fixed << std::setprecision(1)
                  << incl_tx << " deg (estimated from latitude)" << '\n';
        std::cout << "  RX Station:" << '\n';
        std::cout << "    => TEC: " << iono.vTEC_Home << " TECU (typical)" << '\n';
        std::cout << "    => Magnetic inclination: " << incl_rx << " deg (estimated from latitude)" << '\n';

    } else if (choice == 3) {
        std::cout << "\nIf you have measured or downloaded ionosphere data:\n" << '\n';

        std::cout << "TX Station Ionosphere:" << '\n';
        iono.vTEC_DX = getDouble("  Vertical TEC (TECU, typical: 10-50)", 25.0);
        iono.hmF2_DX = getDouble("  F2 layer height (km, typical: 300-400)", 350.0);
        iono.B_magnitude_DX = getDouble("  Magnetic field (Tesla, typical: 3e-5 to 6e-5)", 5.0e-5);
        iono.B_inclination_DX = ParameterUtils::deg2rad(
            getDouble("  Magnetic inclination (degrees, 0=equator, 90=pole)", 60.0));
        iono.B_declination_DX = ParameterUtils::deg2rad(
            getDouble("  Magnetic declination (degrees)", 0.0));

        std::cout << "\nRX Station Ionosphere:" << '\n';
        iono.vTEC_Home = getDouble("  Vertical TEC (TECU)", 25.0);
        iono.hmF2_Home = getDouble("  F2 layer height (km)", 350.0);
        iono.B_magnitude_Home = getDouble("  Magnetic field (Tesla)", 5.0e-5);
        iono.B_inclination_Home = ParameterUtils::deg2rad(
            getDouble("  Magnetic inclination (degrees)", 60.0));
        iono.B_declination_Home = ParameterUtils::deg2rad(
            getDouble("  Magnetic declination (degrees)", 0.0));

        iono.dataSource = "Manual Input";
    }

    std::cout << '\n';
}

void displayResults(const LinkBudgetResults& results) {
    if (!results.calculationSuccess) {
        std::cout << "\n[X] Calculation Failed: " << results.errorMessage << '\n';
        return;
    }

    printHeader("EME Link Budget Results");

    std::cout << "\n[*] Geometry & Moon Position:" << '\n';
    std::cout << "  Moon RA/DEC: " << std::fixed << std::setprecision(2)
              << results.geometry.moonRA_deg << " deg / "
              << results.geometry.moonDEC_deg << " deg" << '\n';
    std::cout << "  Moon Distance: " << results.geometry.moonDistance_km << " km" << '\n';
    std::cout << "  TX Elevation: " << results.geometry.moonElevation_TX_deg << " deg" << '\n';
    std::cout << "  RX Elevation: " << results.geometry.moonElevation_RX_deg << " deg" << '\n';
    std::cout << "  Path Length: " << results.geometry.totalPathLength_km << " km" << '\n';

    if (results.geometry.spectralSpread_Hz > 0.0) {
        std::cout << "\n[*] Spectral Spreading (Libration Effects):" << '\n';
        std::cout << "  Doppler Spread: " << std::setprecision(3)
                  << results.geometry.spectralSpread_Hz << " Hz" << '\n';
        std::cout << "  Coherent Integration Limit: " << std::setprecision(3)
                  << results.geometry.coherentIntegrationLimit_s << " s" << '\n';
        std::cout << "  Libration Velocity: " << std::setprecision(2)
                  << results.geometry.librationVelocity_m_s << " m/s" << '\n';
    }

    std::cout << "\n[*] Path Loss Analysis:" << '\n';
    std::cout << "  Free Space Loss: " << results.pathLoss.freeSpaceLoss_dB << " dB" << '\n';
    std::cout << "  Lunar Scattering: " << results.pathLoss.lunarScatteringLoss_dB << " dB";
    if (results.pathLoss.useHagforsModel) {
        std::cout << " (Hagfors' Law)" << '\n';
        std::cout << "    - Bistatic Angle: " << std::setprecision(2)
                  << results.pathLoss.bistaticAngle_deg << " deg" << '\n';
        std::cout << "    - Roughness Param: " << std::setprecision(3)
                  << results.pathLoss.hagforsRoughnessParam << '\n';
        std::cout << "    - Lunar RCS: " << std::setprecision(2)
                  << results.pathLoss.lunarRCS_dBsm << " dBsm" << '\n';
    } else {
        std::cout << " (Simple Model)" << '\n';
    }
    std::cout << "  Atmospheric Loss: " << results.pathLoss.atmosphericLoss_Total_dB << " dB" << '\n';
    std::cout << "  Total Path Loss: " << results.pathLoss.totalPathLoss_dB << " dB" << '\n';

    std::cout << "\n[*] Polarization Analysis:" << '\n';
    std::cout << "  Spatial Rotation: " << std::setprecision(3)
              << results.polarization.spatialRotation_deg << " deg" << '\n';
    std::cout << "  Faraday Rotation (TX): " << results.polarization.faradayRotation_TX_deg << " deg" << '\n';
    std::cout << "  Faraday Rotation (RX): " << results.polarization.faradayRotation_RX_deg << " deg" << '\n';
    std::cout << "  Total Rotation: " << results.polarization.totalRotation_deg << " deg" << '\n';
    std::cout << "  Polarization Loss: " << std::setprecision(2)
              << results.polarization.polarizationLoss_dB << " dB" << '\n';
    std::cout << "  PLF: " << std::setprecision(6) << results.polarization.PLF << '\n';

    std::cout << "\n[*] Noise Analysis:" << '\n';
    std::cout << "  Sky Noise: " << std::setprecision(1)
              << results.noise.skyNoiseTemp_K << " K";

    if (g_skyModel.isMapLoaded()) {
        std::cout << " (Haslam 408 MHz map)" << '\n';
    } else {
        std::cout << " (Simplified model)" << '\n';
    }

    std::cout << "  Ground Spillover: " << results.noise.groundSpilloverTemp_K << " K" << '\n';
    std::cout << "  System Noise: " << results.noise.systemNoiseTemp_K << " K" << '\n';
    std::cout << "  Noise Power: " << std::setprecision(2)
              << results.noise.noisePower_dBm << " dBm" << '\n';

    std::cout << "\n[*] Signal-to-Noise Ratio:" << '\n';
    std::cout << "  Received Power: " << results.snr.receivedSignalPower_dBm << " dBm" << '\n';
    std::cout << "  SNR: " << results.snr.SNR_dB << " dB" << '\n';
    std::cout << "  Fading Margin: " << results.snr.fadingMargin_dB << " dB" << '\n';
    std::cout << "  Effective SNR: " << results.snr.effectiveSNR_dB << " dB" << '\n';
    std::cout << "  Required SNR: " << results.snr.requiredSNR_dB << " dB (Q65 + AP decode)" << '\n';

    printSeparator('-', 80);
    std::cout << "\n[*] LINK MARGIN: " << std::setprecision(2)
              << results.snr.linkMargin_dB << " dB" << '\n';

    if (results.snr.linkViable) {
        std::cout << "[OK] Link Status: VIABLE - QSO possible!" << '\n';
    } else {
        std::cout << "[X] Link Status: NOT VIABLE - Insufficient margin" << '\n';
    }

    printSeparator();
}

int main() {
    InteractiveRuntimeConfig runtimeConfig;
    std::string loadedConfigPath;
    std::string configError;
    const bool loadedConfig = tryLoadInteractiveRuntimeConfig(runtimeConfig, loadedConfigPath, configError);

    if (!loadedConfig && !configError.empty()) {
        std::cerr << "Error: Failed to load interactive config: " << configError << '\n';
        return 1;
    }

    if (loadedConfig) {
        std::cout << "[OK] Loaded interactive config: " << loadedConfigPath << '\n';
    }

    while (true) {
        printHeader("EME Link Budget Calculator - Interactive Mode");
        std::cout << "Complete EME Link Analysis with User Input\n" << '\n';

        std::cout << "[*] Loading Haslam 408 MHz Sky Map..." << '\n';

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
                std::cout << "[+] Haslam sky map loaded successfully" << '\n';
                break;
            }
        }

        if (!haslamLoaded) {
            std::cout << "[!] Could not load Haslam sky map, using simplified model" << '\n';
        }
        std::cout << '\n';

        LinkBudgetParameters params;
        applyInteractiveDefaults(runtimeConfig, params);

        params.observationTime = inputObservationTime();

        inputStationData("TX (DX)", params.txSite);

        inputStationData("RX (Home)", params.rxSite);

        inputSystemConfiguration(params);

        inputMoonEphemeris(params.moonEphemeris, params.observationTime, params.txSite);

        inputIonosphereData(params.ionosphereData, params.observationTime,
                            params.txSite, params.rxSite);

        printHeader("Calculation Options");
        std::cout << "Enable advanced physical effects (recommended: all yes):\n" << '\n';
        params.includeFaradayRotation = getYesNo("Include Faraday rotation (ionosphere effect)");
        params.includeSpatialRotation = getYesNo("Include spatial rotation (geometry effect)");
        params.includeMoonReflection = getYesNo("Include moon reflection (polarization flip)");
        params.includeAtmosphericLoss = getYesNo("Include atmospheric loss");
        params.includeGroundSpillover = getYesNo("Include ground spillover noise");
        params.useHagforsModel = getYesNo("Use Hagfors' Law for lunar scattering (recommended)");
        std::cout << '\n';

        std::cout << "Calculating link budget..." << '\n';
        EMELinkBudget linkBudget(params);

        std::string validationError;
        if (!linkBudget.validateParameters(validationError)) {
            std::cerr << "[X] Parameter validation failed: " << validationError << '\n';
            if (!getYesNo("Re-enter parameters and try again")) {
                break;
            }
            std::cout << "\n\n";
            continue;
        }

        LinkBudgetResults results = linkBudget.calculate();

        displayResults(results);

        std::cout << "\n";
        if (!getYesNo("Calculate another link")) {
            break;
        }
        std::cout << "\n\n";
    }

    std::cout << "\nThank you for using EME Link Budget Calculator!" << '\n';
    std::cout << "\nTips for better accuracy:" << '\n';
    std::cout << "  * Use real moon position from JPL Horizons or WSJT-X" << '\n';
    std::cout << "  * Use real-time TEC data from IONEX files" << '\n';
    std::cout << "  * Measure your actual system parameters" << '\n';
    std::cout << "  * Check results during actual EME QSOs" << '\n';

    return 0;
}
