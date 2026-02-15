#define _USE_MATH_DEFINES
#include "WMMModel.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WMMModel::WMMModel() : m_loaded(false) {
}

bool WMMModel::loadCoefficientFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    m_coefficients.clear();
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        GaussCoefficient coef;

        if (iss >> coef.n >> coef.m >> coef.gnm >> coef.hnm >> coef.dgnm >> coef.dhnm) {
            if (coef.n <= WMMConstants::MAX_DEGREE) {
                m_coefficients.push_back(coef);
            }
        }
    }

    m_loaded = !m_coefficients.empty();
    return m_loaded;
}

void WMMModel::timeEvolveCoefficients(double decimal_year,
                                      std::vector<double>& g,
                                      std::vector<double>& h) const {
    double dt = decimal_year - WMMConstants::EPOCH;

    int maxIndex = (WMMConstants::MAX_DEGREE + 1) * (WMMConstants::MAX_DEGREE + 2) / 2;
    g.resize(maxIndex, 0.0);
    h.resize(maxIndex, 0.0);

    for (const auto& coef : m_coefficients) {
        int idx = getIndex(coef.n, coef.m);
        g[idx] = coef.gnm + dt * coef.dgnm;
        h[idx] = coef.hnm + dt * coef.dhnm;
    }
}

void WMMModel::geodeticToGeocentric(double lat_deg, double height_km,
                                    double& lat_geocentric_deg,
                                    double& radius_km) const {
    double lat_rad = lat_deg * M_PI / 180.0;
    double sin_lat = std::sin(lat_rad);
    double cos_lat = std::cos(lat_rad);

    double a = WMMConstants::WGS84_A;
    double e2 = WMMConstants::WGS84_E2;

    double N = a / std::sqrt(1.0 - e2 * sin_lat * sin_lat);

    double x = (N + height_km) * cos_lat;
    double z = (N * (1.0 - e2) + height_km) * sin_lat;

    radius_km = std::sqrt(x * x + z * z);
    lat_geocentric_deg = std::atan2(z, x) * 180.0 / M_PI;
}

void WMMModel::computeLegendrePolynomials(double theta,
                                          std::vector<std::vector<double>>& P,
                                          std::vector<std::vector<double>>& dP) const {
    int nMax = WMMConstants::MAX_DEGREE;

    P.assign(nMax + 1, std::vector<double>(nMax + 1, 0.0));
    dP.assign(nMax + 1, std::vector<double>(nMax + 1, 0.0));

    double cos_theta = std::cos(theta);
    double sin_theta = std::sin(theta);

    if (std::abs(sin_theta) < 1e-10) {
        sin_theta = 1e-10;
    }

    std::vector<std::vector<double>> k(nMax + 1, std::vector<double>(nMax + 1, 0.0));

    k[1][0] = 0.0;
    k[1][1] = 0.0;

    for (int n = 2; n <= nMax; ++n) {
        for (int m = 0; m <= n; ++m) {
            if (n == m) {
                k[n][m] = 0.0;
            } else if (m == 0) {
                k[n][m] = ((n - 1) * (n - 1) - m * m) /
                          ((2.0 * n - 1) * (2.0 * n - 3));
            } else {
                k[n][m] = ((n - 1) * (n - 1) - m * m) /
                          ((2.0 * n - 1) * (2.0 * n - 3));
            }
        }
    }

    P[0][0] = 1.0;
    dP[0][0] = 0.0;

    P[1][0] = cos_theta;
    dP[1][0] = -sin_theta;

    P[1][1] = sin_theta;
    dP[1][1] = cos_theta;

    for (int n = 2; n <= nMax; ++n) {
        for (int m = 0; m <= n; ++m) {
            if (n == m) {
                P[n][n] = sin_theta * P[n-1][n-1];
                dP[n][n] = sin_theta * dP[n-1][n-1] + cos_theta * P[n-1][n-1];
            } else if (n == 1) {
                P[n][m] = cos_theta * P[n-1][m];
                dP[n][m] = cos_theta * dP[n-1][m] - sin_theta * P[n-1][m];
            } else if (m == n - 1) {
                P[n][m] = cos_theta * P[n-1][m];
                dP[n][m] = cos_theta * dP[n-1][m] - sin_theta * P[n-1][m];
            } else {
                P[n][m] = cos_theta * P[n-1][m] - k[n][m] * P[n-2][m];
                dP[n][m] = cos_theta * dP[n-1][m] - sin_theta * P[n-1][m] - k[n][m] * dP[n-2][m];
            }
        }
    }

    std::vector<std::vector<double>> schmidtQuasiNorm(nMax + 1, std::vector<double>(nMax + 1, 1.0));

    for (int n = 1; n <= nMax; ++n) {
        schmidtQuasiNorm[n][0] = schmidtQuasiNorm[n-1][0] * (2.0 * n - 1) / n;

        for (int m = 1; m <= n; ++m) {
            schmidtQuasiNorm[n][m] = schmidtQuasiNorm[n][m-1] *
                std::sqrt((n - m + 1) * (m == 1 ? 2.0 : 1.0) / (n + m));
        }
    }

    for (int n = 1; n <= nMax; ++n) {
        for (int m = 0; m <= n; ++m) {
            P[n][m] *= schmidtQuasiNorm[n][m];
            dP[n][m] *= schmidtQuasiNorm[n][m];
        }
    }
}

void WMMModel::computeMagneticField(double r, double theta, double phi,
                                    const std::vector<double>& g,
                                    const std::vector<double>& h,
                                    double& Br, double& Btheta, double& Bphi) const {
    int nMax = WMMConstants::MAX_DEGREE;
    double a = WMMConstants::WGS84_A;

    std::vector<std::vector<double>> P, dP;
    computeLegendrePolynomials(theta, P, dP);

    double sin_theta = std::sin(theta);
    if (std::abs(sin_theta) < 1e-10) {
        sin_theta = 1e-10;
    }

    std::vector<double> cos_m_phi(nMax + 1);
    std::vector<double> sin_m_phi(nMax + 1);

    cos_m_phi[0] = 1.0;
    sin_m_phi[0] = 0.0;

    if (nMax >= 1) {
        cos_m_phi[1] = std::cos(phi);
        sin_m_phi[1] = std::sin(phi);
    }

    for (int m = 2; m <= nMax; ++m) {
        cos_m_phi[m] = cos_m_phi[m-1] * cos_m_phi[1] - sin_m_phi[m-1] * sin_m_phi[1];
        sin_m_phi[m] = sin_m_phi[m-1] * cos_m_phi[1] + cos_m_phi[m-1] * sin_m_phi[1];
    }

    Br = 0.0;
    Btheta = 0.0;
    Bphi = 0.0;

    for (int n = 1; n <= nMax; ++n) {
        double ratio = std::pow(a / r, n + 2);

        for (int m = 0; m <= n; ++m) {
            int idx = getIndex(n, m);
            double gnm = g[idx];
            double hnm = h[idx];

            double cos_term = gnm * cos_m_phi[m] + hnm * sin_m_phi[m];
            double d_lambda_term = hnm * cos_m_phi[m] - gnm * sin_m_phi[m];

            Br += ratio * (n + 1) * P[n][m] * cos_term;
            Btheta += ratio * dP[n][m] * cos_term;

            if (m > 0) {
                Bphi += ratio * m * P[n][m] * d_lambda_term / sin_theta;
            }
        }
    }
}

void WMMModel::rotateToGeodetic(double X_prime, double Z_prime,
                                double lat_geodetic, double lat_geocentric,
                                double& X, double& Z) const {
    double psi = (lat_geodetic - lat_geocentric) * M_PI / 180.0;

    double cos_psi = std::cos(psi);
    double sin_psi = std::sin(psi);

    X = X_prime * cos_psi - Z_prime * sin_psi;
    Z = X_prime * sin_psi + Z_prime * cos_psi;
}

MagneticFieldResult WMMModel::calculate(
    double latitude_deg,
    double longitude_deg,
    double height_km,
    double decimal_year) const {

    MagneticFieldResult result = {};

    if (!m_loaded) {
        return result;
    }

    if (std::abs(latitude_deg) > 89.9) {
        latitude_deg = (latitude_deg > 0) ? 89.9 : -89.9;
    }

    std::vector<double> g, h;
    timeEvolveCoefficients(decimal_year, g, h);

    double lat_geocentric, radius_km;
    geodeticToGeocentric(latitude_deg, height_km, lat_geocentric, radius_km);

    double theta = (90.0 - lat_geocentric) * M_PI / 180.0;
    double phi = longitude_deg * M_PI / 180.0;

    double Br, Btheta, Bphi;
    computeMagneticField(radius_km, theta, phi, g, h, Br, Btheta, Bphi);

    double X_gc = Btheta;
    double Y_gc = -Bphi;
    double Z_gc = -Br;

    double X, Z;
    rotateToGeodetic(X_gc, Z_gc, latitude_deg, lat_geocentric, X, Z);

    result.X = X;
    result.Y = Y_gc;
    result.Z = Z;
    result.H = std::sqrt(X * X + Y_gc * Y_gc);
    result.F = std::sqrt(X * X + Y_gc * Y_gc + Z * Z);
    result.inclination = std::atan2(Z, result.H) * 180.0 / M_PI;
    result.declination = std::atan2(Y_gc, X) * 180.0 / M_PI;

    return result;
}

int WMMModel::getIndex(int n, int m) const {
    return n * (n + 1) / 2 + m;
}
