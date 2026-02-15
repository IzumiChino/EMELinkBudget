#pragma once

#include "Parameters.h"
#include "MaidenheadGrid.h"
#include "IonospherePhysics.h"
#include <complex>
#include <array>
#include <memory>

using JonesVector = std::array<std::complex<double>, 2>;
using Matrix2x2 = std::array<std::array<std::complex<double>, 2>, 2>;

// ========== Faraday Rotation Calculator ==========
class FaradayRotation {
public:
    FaradayRotation();
    explicit FaradayRotation(const SystemConfiguration& config);

    // ========== Parameter Setup ==========
    void setConfiguration(const SystemConfiguration& config);
    void setDXStation(double latitude, double longitude, double psi, double chi);
    void setHomeStation(double latitude, double longitude, double psi, double chi);
    void setDXStationByGrid(const std::string& grid, double psi, double chi);
    void setHomeStationByGrid(const std::string& grid, double psi, double chi);
    void setDXStation(const SiteParameters& site);
    void setHomeStation(const SiteParameters& site);
    void setIonosphereData(const IonosphereData& iono);
    void setMoonEphemeris(const MoonEphemeris& moon);

    // ========== Main Calculation ==========
    CalculationResults calculate();
    const CalculationResults& getLastResults() const { return m_lastResults; }

    // ========== Helper Calculations ==========
    double calculateParallacticAngle(
        double latitude, double declination, double hourAngle) const;
    double calculateFaradayRotation(
        double vTEC, double B_magnitude, double B_inclination, double B_declination,
        double elevation, double azimuth) const;
    double calculateSlantFactor(double elevation) const;
    double calculateMagneticAngle(
        double B_inclination, double B_declination,
        double elevation, double azimuth) const;

    // ========== Jones Calculus Operations ==========
    JonesVector createJonesVector(double psi, double chi) const;
    Matrix2x2 createRotationMatrix(double angle) const;
    Matrix2x2 createMoonReflectionMatrix() const;
    JonesVector matrixVectorMultiply(const Matrix2x2& mat, const JonesVector& vec) const;
    Matrix2x2 matrixMultiply(const Matrix2x2& A, const Matrix2x2& B) const;
    std::complex<double> vectorDotProduct(const JonesVector& a, const JonesVector& b) const;

    // ========== Information Query ==========
    const SystemConfiguration& getConfiguration() const { return m_config; }
    const SiteParameters& getDXStation() const { return m_dxSite; }
    const SiteParameters& getHomeStation() const { return m_homeSite; }
    const IonosphereData& getIonosphereData() const { return m_ionoData; }
    const MoonEphemeris& getMoonEphemeris() const { return m_moonEphem; }
    double calculateStationDistance() const;
    bool validateParameters(std::string& errorMsg) const;

private:
    SystemConfiguration m_config;
    SiteParameters m_dxSite;
    SiteParameters m_homeSite;
    IonosphereData m_ionoData;
    MoonEphemeris m_moonEphem;
    CalculationResults m_lastResults;

    void calculateMoonElevation();
    double calculatePathLength() const;
    double normalizeAngle(double angle) const;
    double deg2rad(double degrees) const;
    double rad2deg(double radians) const;
};
