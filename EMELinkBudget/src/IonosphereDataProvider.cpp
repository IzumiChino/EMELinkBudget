#define _USE_MATH_DEFINES
#include "IonosphereDataProvider.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== Constructor ==========

IonosphereDataProvider::IonosphereDataProvider()
    : m_reader(nullptr), m_wmm(nullptr),
      m_ionexLoaded(false), m_wmmLoaded(false) {
}

// ========== File Loading ==========

bool IonosphereDataProvider::loadIonexFile(const std::string& filename) {
    m_reader = std::make_unique<IonexReader>(filename);
    m_ionexLoaded = m_reader->isOpen();
    return m_ionexLoaded;
}

bool IonosphereDataProvider::loadWMMFile(const std::string& filename) {
    m_wmm = std::make_unique<WMMModel>();
    m_wmmLoaded = m_wmm->loadCoefficientFile(filename);
    return m_wmmLoaded;
}

// ========== Time Conversion ==========

double IonosphereDataProvider::tmToDecimalYear(const std::tm& time) const {
    int year = time.tm_year + 1900;
    int month = time.tm_mon + 1;
    int day = time.tm_mday;
    int hour = time.tm_hour;
    int minute = time.tm_min;

    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int days_in_year = is_leap ? 366 : 365;

    int days_before_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int day_of_year = days_before_month[month - 1] + day;

    if (is_leap && month > 2) {
        day_of_year += 1;
    }

    double fraction = (day_of_year - 1 + hour / 24.0 + minute / 1440.0) / days_in_year;

    return year + fraction;
}

// ========== Ionosphere Data Retrieval ==========

bool IonosphereDataProvider::getIonosphereData(
    const std::tm& time,
    double lat_dx, double lon_dx, double height_dx_km,
    double lat_home, double lon_home, double height_home_km,
    IonosphereData& ionoData) {

    if (!m_ionexLoaded || !m_reader) {
        return false;
    }

    double vtec_dx, vtec_home;

    bool success_dx = m_reader->getTecValueInterpolated(time, lat_dx, lon_dx, vtec_dx);
    bool success_home = m_reader->getTecValueInterpolated(time, lat_home, lon_home, vtec_home);

    if (!success_dx || !success_home) {
        return false;
    }

    ionoData.vTEC_DX = vtec_dx;
    ionoData.vTEC_Home = vtec_home;

    if (m_wmmLoaded && m_wmm) {
        double decimal_year = tmToDecimalYear(time);

        MagneticFieldResult mag_dx = m_wmm->calculate(lat_dx, lon_dx, height_dx_km, decimal_year);
        MagneticFieldResult mag_home = m_wmm->calculate(lat_home, lon_home, height_home_km, decimal_year);

        ionoData.B_magnitude_DX = mag_dx.F * 1e-9;
        ionoData.B_magnitude_Home = mag_home.F * 1e-9;
        ionoData.B_inclination_DX = mag_dx.inclination * M_PI / 180.0;
        ionoData.B_inclination_Home = mag_home.inclination * M_PI / 180.0;
        ionoData.B_declination_DX = mag_dx.declination * M_PI / 180.0;
        ionoData.B_declination_Home = mag_home.declination * M_PI / 180.0;
    } else {
        ionoData.B_magnitude_DX = 5.0e-5;
        ionoData.B_magnitude_Home = 5.0e-5;
        ionoData.B_inclination_DX = 1.047;
        ionoData.B_inclination_Home = 1.047;
        ionoData.B_declination_DX = 0.0;
        ionoData.B_declination_Home = 0.0;
    }

    ionoData.dataSource = m_wmmLoaded ? "IONEX + WMM" : "IONEX + Default Magnetic";
    ionoData.timestamp = std::mktime(const_cast<std::tm*>(&time));

    return true;
}
