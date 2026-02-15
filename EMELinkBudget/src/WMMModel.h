#pragma once

#include <vector>
#include <string>
#include <cmath>

// ========== WMM Constants ==========

namespace WMMConstants {
    constexpr double WGS84_A = 6378.137;
    constexpr double WGS84_F = 1.0 / 298.257223563;
    constexpr double WGS84_B = WGS84_A * (1.0 - WGS84_F);
    constexpr double WGS84_E2 = 2.0 * WGS84_F - WGS84_F * WGS84_F;
    constexpr double EPOCH = 2025.0;
    constexpr int MAX_DEGREE = 12;
}

// ========== Gauss Coefficient ==========

struct GaussCoefficient {
    int n;
    int m;
    double gnm;
    double hnm;
    double dgnm;
    double dhnm;
};

// ========== Magnetic Field Result ==========

struct MagneticFieldResult {
    double X;
    double Y;
    double Z;
    double H;
    double F;
    double inclination;
    double declination;
};

// ========== WMM Model ==========

class WMMModel {
public:
    WMMModel();

    bool loadCoefficientFile(const std::string& filename);

    MagneticFieldResult calculate(
        double latitude_deg,
        double longitude_deg,
        double height_km,
        double decimal_year) const;

private:
    std::vector<GaussCoefficient> m_coefficients;
    bool m_loaded;

    void timeEvolveCoefficients(double decimal_year,
                                std::vector<double>& g,
                                std::vector<double>& h) const;

    void geodeticToGeocentric(double lat_deg, double height_km,
                              double& lat_geocentric_deg,
                              double& radius_km) const;

    void computeLegendrePolynomials(double theta,
                                    std::vector<std::vector<double>>& P,
                                    std::vector<std::vector<double>>& dP) const;

    void computeMagneticField(double r, double theta, double phi,
                              const std::vector<double>& g,
                              const std::vector<double>& h,
                              double& Br, double& Btheta, double& Bphi) const;

    void rotateToGeodetic(double X_prime, double Z_prime,
                          double lat_geodetic, double lat_geocentric,
                          double& X, double& Z) const;

    int getIndex(int n, int m) const;
};
