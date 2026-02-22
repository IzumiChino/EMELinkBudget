#pragma once

#include <string>
#include <ctime>
#include <cmath>

// ========== System Constants ==========
namespace SystemConstants {
    constexpr double PI = 3.14159265358979323846;
    constexpr double FARADAY_CONSTANT = 0.23647;
    constexpr double EARTH_RADIUS_KM = 6371.0;
    constexpr double IONOSPHERE_HEIGHT_KM = 350.0;
    constexpr double SPEED_OF_LIGHT = 299792458.0;
}

// ========== Site Parameters ==========
struct SiteParameters {
    double latitude;
    double longitude;
    std::string gridLocator;
    double psi;
    double chi;
    std::string callsign;
    std::string name;

    SiteParameters()
        : latitude(0.0), longitude(0.0), psi(0.0), chi(0.0),
          gridLocator(""), callsign(""), name("") {}
};

// ========== Ionosphere Data ==========
struct IonosphereData {
    double vTEC_DX;
    double vTEC_Home;
    double hmF2_DX;
    double hmF2_Home;
    double B_magnitude_DX;
    double B_magnitude_Home;
    double B_inclination_DX;
    double B_inclination_Home;
    double B_declination_DX;
    double B_declination_Home;
    std::string dataSource;
    std::time_t timestamp;

    IonosphereData()
        : vTEC_DX(20.0), vTEC_Home(20.0),
          hmF2_DX(350.0), hmF2_Home(350.0),
          B_magnitude_DX(5e-5), B_magnitude_Home(5e-5),
          B_inclination_DX(0.0), B_inclination_Home(0.0),
          B_declination_DX(0.0), B_declination_Home(0.0),
          dataSource("Manual"), timestamp(0) {}
};

// ========== Moon Ephemeris ==========
struct MoonEphemeris {
    double rightAscension;
    double declination;
    double distance_km;
    double hourAngle_DX;
    double hourAngle_Home;
    double azimuth_DX;
    double elevation_DX;
    double azimuth_Home;
    double elevation_Home;
    double rangeRate_km_s;
    double librationLon_deg;
    double librationLat_deg;
    double librationLonRate_deg_day;
    double librationLatRate_deg_day;
    std::time_t observationTime;
    double julianDate;
    std::string ephemerisSource;

    MoonEphemeris()
        : rightAscension(0.0), declination(0.0), distance_km(384400.0),
          hourAngle_DX(0.0), hourAngle_Home(0.0),
          azimuth_DX(0.0), elevation_DX(0.0),
          azimuth_Home(0.0), elevation_Home(0.0),
          rangeRate_km_s(0.0), librationLon_deg(0.0), librationLat_deg(0.0),
          librationLonRate_deg_day(0.0), librationLatRate_deg_day(0.0),
          observationTime(0), julianDate(0.0),
          ephemerisSource("Manual") {}
};

// ========== System Configuration ==========
struct SystemConfiguration {
    double frequency_MHz;
    double bandwidth_Hz;
    bool includeFaradayRotation;
    bool includeSpatialRotation;
    bool includeMoonReflection;

    enum class IonosphereModel {
        SIMPLE,
        CHAPMAN,
        IRI,
        CUSTOM
    };
    IonosphereModel ionoModel;

    enum class MagneticFieldModel {
        DIPOLE,
        IGRF,
        WMM,
        CUSTOM
    };
    MagneticFieldModel magModel;

    SystemConfiguration()
        : frequency_MHz(144.0), bandwidth_Hz(2500.0),
          includeFaradayRotation(true),
          includeSpatialRotation(true),
          includeMoonReflection(true),
          ionoModel(IonosphereModel::SIMPLE),
          magModel(MagneticFieldModel::DIPOLE) {}
};

// ========== Calculation Results ==========
struct CalculationResults {
    double spatialRotation_deg;
    double faradayRotation_DX_deg;
    double faradayRotation_Home_deg;
    double totalRotation_deg;
    double PLF;
    double polarizationLoss_dB;
    double polarizationEfficiency;
    double pathLength_km;
    double propagationDelay_ms;
    double parallacticAngle_DX_deg;
    double parallacticAngle_Home_deg;
    double slantFactor_DX;
    double slantFactor_Home;
    bool calculationSuccess;
    std::string errorMessage;
    std::time_t calculationTime;

    CalculationResults()
        : spatialRotation_deg(0.0),
          faradayRotation_DX_deg(0.0),
          faradayRotation_Home_deg(0.0),
          totalRotation_deg(0.0),
          PLF(0.0),
          polarizationLoss_dB(0.0),
          polarizationEfficiency(0.0),
          pathLength_km(0.0),
          propagationDelay_ms(0.0),
          parallacticAngle_DX_deg(0.0),
          parallacticAngle_Home_deg(0.0),
          slantFactor_DX(1.0),
          slantFactor_Home(1.0),
          calculationSuccess(false),
          errorMessage(""),
          calculationTime(0) {}
};

// ========== Utility Functions ==========
namespace ParameterUtils {
    inline double deg2rad(double degrees) {
        return degrees * SystemConstants::PI / 180.0;
    }

    inline double rad2deg(double radians) {
        return radians * 180.0 / SystemConstants::PI;
    }

    inline std::string getPolarizationType(double chi) {
        const double threshold = 0.01;
        if (std::abs(chi) < threshold) {
            return "Linear";
        } else if (chi > SystemConstants::PI / 4.0 - threshold) {
            return "RHCP";
        } else if (chi < -SystemConstants::PI / 4.0 + threshold) {
            return "LHCP";
        } else if (chi > 0) {
            return "Right Elliptical";
        } else {
            return "Left Elliptical";
        }
    }

    inline std::string getFrequencyBand(double freq_MHz) {
        if (freq_MHz >= 50 && freq_MHz < 54) return "6m";
        if (freq_MHz >= 144 && freq_MHz < 148) return "2m";
        if (freq_MHz >= 420 && freq_MHz < 450) return "70cm";
        if (freq_MHz >= 1240 && freq_MHz < 1300) return "23cm";
		if (freq_MHz >= 2400 && freq_MHz < 2450) return "13cm";
		if (freq_MHz >= 5650 && freq_MHz < 5925) return "6cm";
		if (freq_MHz >= 10000 && freq_MHz < 10500) return "3cm";
		if (freq_MHz >= 24000 && freq_MHz < 24250) return "1.25cm";
		if (freq_MHz >= 47000 && freq_MHz < 47200) return "6mm";
		if (freq_MHz >= 100000 && freq_MHz < 300000) return "mm-wave";
		if (freq_MHz >= 300000) return "Sub-mm";
        return "OOB";
    }
}
