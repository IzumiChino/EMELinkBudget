#include "FaradayRotation.h"
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <iostream>

// ========== Constructors ==========

FaradayRotation::FaradayRotation()
    : m_config(), m_dxSite(), m_homeSite(), m_ionoData(), m_moonEphem(), m_lastResults() {
}

FaradayRotation::FaradayRotation(const SystemConfiguration& config)
    : m_config(config), m_dxSite(), m_homeSite(), m_ionoData(), m_moonEphem(), m_lastResults() {
}

// ========== Parameter Setup ==========

void FaradayRotation::setConfiguration(const SystemConfiguration& config) {
    m_config = config;
}

void FaradayRotation::setDXStation(
    double latitude, double longitude, double psi, double chi) {
    m_dxSite.latitude = latitude;
    m_dxSite.longitude = longitude;
    m_dxSite.psi = psi;
    m_dxSite.chi = chi;
    m_dxSite.gridLocator = MaidenheadGrid::latLonToGrid(
        rad2deg(latitude), rad2deg(longitude), 6);
}

void FaradayRotation::setHomeStation(
    double latitude, double longitude, double psi, double chi) {
    m_homeSite.latitude = latitude;
    m_homeSite.longitude = longitude;
    m_homeSite.psi = psi;
    m_homeSite.chi = chi;
    m_homeSite.gridLocator = MaidenheadGrid::latLonToGrid(
        rad2deg(latitude), rad2deg(longitude), 6);
}

void FaradayRotation::setDXStationByGrid(
    const std::string& grid, double psi, double chi) {
    double lat, lon;
    MaidenheadGrid::gridToLatLon(grid, lat, lon);
    m_dxSite.latitude = deg2rad(lat);
    m_dxSite.longitude = deg2rad(lon);
    m_dxSite.psi = psi;
    m_dxSite.chi = chi;
    m_dxSite.gridLocator = grid;
}

void FaradayRotation::setHomeStationByGrid(
    const std::string& grid, double psi, double chi) {
    double lat, lon;
    MaidenheadGrid::gridToLatLon(grid, lat, lon);
    m_homeSite.latitude = deg2rad(lat);
    m_homeSite.longitude = deg2rad(lon);
    m_homeSite.psi = psi;
    m_homeSite.chi = chi;
    m_homeSite.gridLocator = grid;
}

void FaradayRotation::setDXStation(const SiteParameters& site) {
    m_dxSite = site;
}

void FaradayRotation::setHomeStation(const SiteParameters& site) {
    m_homeSite = site;
}

void FaradayRotation::setIonosphereData(const IonosphereData& iono) {
    m_ionoData = iono;
}

void FaradayRotation::setMoonEphemeris(const MoonEphemeris& moon) {
    m_moonEphem = moon;
}

// ========== Helper Functions ==========

double FaradayRotation::deg2rad(double degrees) const {
    return degrees * SystemConstants::PI / 180.0;
}

double FaradayRotation::rad2deg(double radians) const {
    return radians * 180.0 / SystemConstants::PI;
}

double FaradayRotation::normalizeAngle(double angle) const {
    while (angle > SystemConstants::PI) angle -= 2.0 * SystemConstants::PI;
    while (angle < -SystemConstants::PI) angle += 2.0 * SystemConstants::PI;
    return angle;
}

// ========== Parameter Validation ==========

bool FaradayRotation::validateParameters(std::string& errorMsg) const {
    std::ostringstream oss;

    if (m_config.frequency_MHz <= 0) {
        oss << "Invalid frequency: " << m_config.frequency_MHz << " MHz. ";
    }

    if (std::abs(m_dxSite.latitude) > SystemConstants::PI / 2.0) {
        oss << "DX latitude out of range. ";
    }

    if (std::abs(m_homeSite.latitude) > SystemConstants::PI / 2.0) {
        oss << "Home latitude out of range. ";
    }

    if (m_ionoData.vTEC_DX < 0 || m_ionoData.vTEC_Home < 0) {
        oss << "vTEC values must be non-negative. ";
    }

    if (m_ionoData.B_magnitude_DX <= 0 || m_ionoData.B_magnitude_Home <= 0) {
        oss << "Magnetic field magnitude must be positive. ";
    }

    errorMsg = oss.str();
    return errorMsg.empty();
}

// ========== Distance Calculation ==========

double FaradayRotation::calculateStationDistance() const {
    return MaidenheadGrid::calculateDistanceLatLon(
        rad2deg(m_dxSite.latitude), rad2deg(m_dxSite.longitude),
        rad2deg(m_homeSite.latitude), rad2deg(m_homeSite.longitude)
    );
}

double FaradayRotation::calculatePathLength() const {
    return 2.0 * m_moonEphem.distance_km;
}

// ========== Moon Elevation Calculation ==========

void FaradayRotation::calculateMoonElevation() {
    if (m_moonEphem.elevation_DX != 0.0 || m_moonEphem.elevation_Home != 0.0) {
        return;
    }

    double sinLat_DX = std::sin(m_dxSite.latitude);
    double cosLat_DX = std::cos(m_dxSite.latitude);
    double sinDec = std::sin(m_moonEphem.declination);
    double cosDec = std::cos(m_moonEphem.declination);
    double cosH_DX = std::cos(m_moonEphem.hourAngle_DX);

    m_moonEphem.elevation_DX = std::asin(
        sinLat_DX * sinDec + cosLat_DX * cosDec * cosH_DX
    );

    double sinLat_Home = std::sin(m_homeSite.latitude);
    double cosLat_Home = std::cos(m_homeSite.latitude);
    double cosH_Home = std::cos(m_moonEphem.hourAngle_Home);

    m_moonEphem.elevation_Home = std::asin(
        sinLat_Home * sinDec + cosLat_Home * cosDec * cosH_Home
    );

    double sinH_DX = std::sin(m_moonEphem.hourAngle_DX);
    double tanDec = std::tan(m_moonEphem.declination);

    m_moonEphem.azimuth_DX = std::atan2(
        sinH_DX,
        cosH_DX * sinLat_DX - tanDec * cosLat_DX
    );

    double sinH_Home = std::sin(m_moonEphem.hourAngle_Home);
    m_moonEphem.azimuth_Home = std::atan2(
        sinH_Home,
        cosH_Home * sinLat_Home - tanDec * cosLat_Home
    );
}

// ========== Parallactic Angle Calculation ==========

double FaradayRotation::calculateParallacticAngle(
    double latitude, double declination, double hourAngle) const {

    double sinH = std::sin(hourAngle);
    double cosH = std::cos(hourAngle);
    double sinLat = std::sin(latitude);
    double cosLat = std::cos(latitude);
    double sinDec = std::sin(declination);
    double cosDec = std::cos(declination);

    double numerator = sinH * cosLat;
    double denominator = sinLat * cosDec - cosLat * sinDec * cosH;

    return std::atan2(numerator, denominator);
}

// ========== Slant Factor Calculation ==========

double FaradayRotation::calculateSlantFactor(double elevation) const {
    if (elevation < 0) {
        return 1.0;
    }

    double hmF2 = SystemConstants::IONOSPHERE_HEIGHT_KM;
    return IonospherePhysics::calculateMappingFunction(elevation, hmF2);
}

// ========== Magnetic Angle Calculation ==========

double FaradayRotation::calculateMagneticAngle(
    double B_inclination, double B_declination,
    double elevation, double azimuth) const {

    double prop_x = std::cos(elevation) * std::cos(azimuth);
    double prop_y = std::cos(elevation) * std::sin(azimuth);
    double prop_z = std::sin(elevation);

    double B_x = std::cos(B_inclination) * std::cos(B_declination);
    double B_y = std::cos(B_inclination) * std::sin(B_declination);
    double B_z = -std::sin(B_inclination);

    double dotProduct = prop_x * B_x + prop_y * B_y + prop_z * B_z;

    return std::acos(std::max(-1.0, std::min(1.0, dotProduct)));
}

// ========== Faraday Rotation Calculation ==========

double FaradayRotation::calculateFaradayRotation(
    double vTEC, double B_magnitude, double B_inclination, double B_declination,
    double elevation, double azimuth) const {

    double f_MHz = m_config.frequency_MHz;

    double hmF2 = SystemConstants::IONOSPHERE_HEIGHT_KM;

    double omega = IonospherePhysics::calculateFaradayRotationPrecise(
        vTEC, hmF2,
        B_magnitude, B_inclination, B_declination,
        elevation, azimuth,
        f_MHz
    );

    return omega;
}

// ========== Jones Calculus Operations ==========

JonesVector FaradayRotation::createJonesVector(double psi, double chi) const {
    double cosPsi = std::cos(psi);
    double sinPsi = std::sin(psi);
    double cosChi = std::cos(chi);
    double sinChi = std::sin(chi);

    JonesVector J;
    J[0] = std::complex<double>(cosPsi * cosChi, -sinPsi * sinChi);
    J[1] = std::complex<double>(sinPsi * cosChi, cosPsi * sinChi);

    return J;
}

Matrix2x2 FaradayRotation::createRotationMatrix(double angle) const {
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);

    Matrix2x2 R;
    R[0][0] = std::complex<double>(cosAngle, 0.0);
    R[0][1] = std::complex<double>(-sinAngle, 0.0);
    R[1][0] = std::complex<double>(sinAngle, 0.0);
    R[1][1] = std::complex<double>(cosAngle, 0.0);

    return R;
}

Matrix2x2 FaradayRotation::createMoonReflectionMatrix() const {
    Matrix2x2 M;
    M[0][0] = std::complex<double>(1.0, 0.0);
    M[0][1] = std::complex<double>(0.0, 0.0);
    M[1][0] = std::complex<double>(0.0, 0.0);
    M[1][1] = std::complex<double>(-1.0, 0.0);

    return M;
}

JonesVector FaradayRotation::matrixVectorMultiply(
    const Matrix2x2& mat, const JonesVector& vec) const {

    JonesVector result;
    result[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1];
    result[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1];

    return result;
}

Matrix2x2 FaradayRotation::matrixMultiply(
    const Matrix2x2& A, const Matrix2x2& B) const {

    Matrix2x2 result;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            result[i][j] = A[i][0] * B[0][j] + A[i][1] * B[1][j];
        }
    }

    return result;
}

std::complex<double> FaradayRotation::vectorDotProduct(
    const JonesVector& a, const JonesVector& b) const {
    return std::conj(a[0]) * b[0] + std::conj(a[1]) * b[1];
}

// ========== Main Calculation Function ==========

CalculationResults FaradayRotation::calculate() {
    m_lastResults = CalculationResults();
    m_lastResults.calculationTime = std::time(nullptr);

    std::string errorMsg;
    if (!validateParameters(errorMsg)) {
        m_lastResults.calculationSuccess = false;
        m_lastResults.errorMessage = errorMsg;
        return m_lastResults;
    }

    try {
        calculateMoonElevation();

        if (m_moonEphem.elevation_DX < 0 || m_moonEphem.elevation_Home < 0) {
            m_lastResults.calculationSuccess = false;
            m_lastResults.errorMessage = "Moon is below horizon at one or both stations";
            return m_lastResults;
        }

        double nu_DX = calculateParallacticAngle(
            m_dxSite.latitude,
            m_moonEphem.declination,
            m_moonEphem.hourAngle_DX
        );

        double nu_Home = calculateParallacticAngle(
            m_homeSite.latitude,
            m_moonEphem.declination,
            m_moonEphem.hourAngle_Home
        );

        m_lastResults.parallacticAngle_DX_deg = rad2deg(nu_DX);
        m_lastResults.parallacticAngle_Home_deg = rad2deg(nu_Home);

        double spatialRotation = 0.0;
        if (m_config.includeSpatialRotation) {
            spatialRotation = nu_DX + nu_Home;
        }
        m_lastResults.spatialRotation_deg = rad2deg(spatialRotation);

        m_lastResults.slantFactor_DX = calculateSlantFactor(m_moonEphem.elevation_DX);
        m_lastResults.slantFactor_Home = calculateSlantFactor(m_moonEphem.elevation_Home);

        double faradayRotation_DX = 0.0;
        double faradayRotation_Home = 0.0;

        if (m_config.includeFaradayRotation) {
            faradayRotation_DX = IonospherePhysics::calculateFaradayRotationPrecise(
                m_ionoData.vTEC_DX,
                m_ionoData.hmF2_DX,
                m_ionoData.B_magnitude_DX,
                m_ionoData.B_inclination_DX,
                m_ionoData.B_declination_DX,
                m_moonEphem.elevation_DX,
                m_moonEphem.azimuth_DX,
                m_config.frequency_MHz
            );

            faradayRotation_Home = IonospherePhysics::calculateFaradayRotationPrecise(
                m_ionoData.vTEC_Home,
                m_ionoData.hmF2_Home,
                m_ionoData.B_magnitude_Home,
                m_ionoData.B_inclination_Home,
                m_ionoData.B_declination_Home,
                m_moonEphem.elevation_Home,
                m_moonEphem.azimuth_Home,
                m_config.frequency_MHz
            );
        }

        m_lastResults.faradayRotation_DX_deg = rad2deg(faradayRotation_DX);
        m_lastResults.faradayRotation_Home_deg = rad2deg(faradayRotation_Home);

        double totalRotation = spatialRotation + faradayRotation_DX + faradayRotation_Home;
        m_lastResults.totalRotation_deg = rad2deg(totalRotation);

        JonesVector J_TX = createJonesVector(m_dxSite.psi, m_dxSite.chi);
        JonesVector J_RX = createJonesVector(m_homeSite.psi, m_homeSite.chi);

        double Phi_up = nu_DX + faradayRotation_DX;
        double Phi_down = nu_Home + faradayRotation_Home;

        // Debug output for polarization analysis
        std::cout << "\n[DEBUG] Polarization Calculation Details:\n";
        std::cout << "  TX psi: " << rad2deg(m_dxSite.psi) << " deg, chi: " << rad2deg(m_dxSite.chi) << " deg\n";
        std::cout << "  RX psi: " << rad2deg(m_homeSite.psi) << " deg, chi: " << rad2deg(m_homeSite.chi) << " deg\n";
        std::cout << "  Parallactic angle TX: " << rad2deg(nu_DX) << " deg\n";
        std::cout << "  Parallactic angle RX: " << rad2deg(nu_Home) << " deg\n";
        std::cout << "  Phi_up (TX rotation): " << rad2deg(Phi_up) << " deg\n";
        std::cout << "  Phi_down (RX rotation): " << rad2deg(Phi_down) << " deg\n";
        std::cout << "  Total rotation: " << rad2deg(totalRotation) << " deg\n";

        Matrix2x2 R_up = createRotationMatrix(Phi_up);
        Matrix2x2 M_moon = m_config.includeMoonReflection ?
                          createMoonReflectionMatrix() :
                          createRotationMatrix(0.0);
        Matrix2x2 R_down = createRotationMatrix(Phi_down);

        JonesVector E1 = matrixVectorMultiply(R_up, J_TX);
        JonesVector E2 = matrixVectorMultiply(M_moon, E1);
        JonesVector E_final = matrixVectorMultiply(R_down, E2);

        // Calculate Polarization Loss Factor using Jones calculus
        // PLF = |J_RX† · E_final|²
        // This naturally handles all polarization types (linear, circular, elliptical)
        // and automatically accounts for 180° symmetry in linear polarization
        // because |a|² = |-a|²
        std::complex<double> innerProduct = vectorDotProduct(J_RX, E_final);
        double PLF = std::norm(innerProduct);

        std::cout << "  J_TX: [" << J_TX[0] << ", " << J_TX[1] << "]\n";
        std::cout << "  J_RX: [" << J_RX[0] << ", " << J_RX[1] << "]\n";
        std::cout << "  E_final: [" << E_final[0] << ", " << E_final[1] << "]\n";
        std::cout << "  Inner product: " << innerProduct << "\n";
        std::cout << "  PLF: " << PLF << "\n\n";

        m_lastResults.PLF = PLF;
        m_lastResults.polarizationLoss_dB = -10.0 * std::log10(PLF);  // Loss is positive when PLF < 1
        m_lastResults.polarizationEfficiency = PLF * 100.0;

        m_lastResults.pathLength_km = calculatePathLength();
        m_lastResults.propagationDelay_ms =
            (m_lastResults.pathLength_km * 1000.0) / SystemConstants::SPEED_OF_LIGHT * 1000.0;

        m_lastResults.calculationSuccess = true;

    } catch (const std::exception& e) {
        m_lastResults.calculationSuccess = false;
        m_lastResults.errorMessage = std::string("Calculation error: ") + e.what();
    }

    return m_lastResults;
}

