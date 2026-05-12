#include "IonospherePhysics.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi_v<double>;
constexpr double kDegreesToRadians = kPi / 180.0;
constexpr double kRadiansToDegrees = 180.0 / kPi;

}

int main() {
    std::cout << "Detailed Faraday Rotation Verification\n";
    std::cout << "========================================\n\n";

    std::cout << "User's DX Station Data:\n";
    std::cout << "-----------------------\n";
    double freq_dx = 432.065;
    double vtec_dx = 4.43;
    double hmf2_dx = 350.0;
    double elev_dx = 9.6 * kDegreesToRadians;
    double az_dx = 147.6 * kDegreesToRadians;
    double B_mag_dx = 52874.339e-9;
    double B_incl_dx = 70.817 * kDegreesToRadians;
    double B_decl_dx = 0.0;

    std::cout << "Frequency: " << freq_dx << " MHz\n";
    std::cout << "vTEC: " << vtec_dx << " TECU\n";
    std::cout << "hmF2: " << hmf2_dx << " km\n";
    std::cout << "Elevation: " << (elev_dx * kRadiansToDegrees) << " deg\n";
    std::cout << "Azimuth: " << (az_dx * kRadiansToDegrees) << " deg\n";
    std::cout << "B magnitude: " << (B_mag_dx * 1e9) << " nT\n";
    std::cout << "B inclination: " << (B_incl_dx * kRadiansToDegrees) << " deg\n";
    std::cout << "B declination: " << (B_decl_dx * kRadiansToDegrees) << " deg\n\n";

    double mf_dx = IonospherePhysics::calculateMappingFunction(elev_dx, hmf2_dx);
    double stec_dx = vtec_dx * mf_dx;

    std::cout << "Mapping Factor: " << std::fixed << std::setprecision(4) << mf_dx << "\n";
    std::cout << "sTEC: " << std::setprecision(2) << stec_dx << " TECU\n\n";

    double B_proj_dx = IonospherePhysics::calculateMagneticFieldProjection(
        B_mag_dx, B_incl_dx, B_decl_dx, elev_dx, az_dx);

    std::cout << "B projected: " << std::scientific << std::setprecision(6) << B_proj_dx << " T\n";
    std::cout << "B projected: " << std::fixed << std::setprecision(2) << (B_proj_dx * 1e9) << " nT\n\n";

    double omega_dx = IonospherePhysics::calculateFaradayRotationPrecise(
        vtec_dx, hmf2_dx, B_mag_dx, B_incl_dx, B_decl_dx, elev_dx, az_dx, freq_dx);

    std::cout << "Faraday Rotation: " << std::setprecision(6) << omega_dx << " rad\n";
    std::cout << "Faraday Rotation: " << std::setprecision(3) << (omega_dx * kRadiansToDegrees) << " deg\n";
    std::cout << "Expected from program: -18.656 deg\n\n";

    std::cout << "========================================\n\n";

    std::cout << "User's Home Station Data:\n";
    std::cout << "-------------------------\n";
    double vtec_home = 25.29;
    double hmf2_home = 350.0;
    double elev_home = 22.4 * kDegreesToRadians;
    double az_home = 224.9 * kDegreesToRadians;
    double B_mag_home = 50616.592e-9;
    double B_incl_home = 49.309 * kDegreesToRadians;
    double B_decl_home = 0.0;

    std::cout << "Frequency: " << freq_dx << " MHz\n";
    std::cout << "vTEC: " << vtec_home << " TECU\n";
    std::cout << "hmF2: " << hmf2_home << " km\n";
    std::cout << "Elevation: " << (elev_home * kRadiansToDegrees) << " deg\n";
    std::cout << "Azimuth: " << (az_home * kRadiansToDegrees) << " deg\n";
    std::cout << "B magnitude: " << (B_mag_home * 1e9) << " nT\n";
    std::cout << "B inclination: " << (B_incl_home * kRadiansToDegrees) << " deg\n";
    std::cout << "B declination: " << (B_decl_home * kRadiansToDegrees) << " deg\n\n";

    double mf_home = IonospherePhysics::calculateMappingFunction(elev_home, hmf2_home);
    double stec_home = vtec_home * mf_home;

    std::cout << "Mapping Factor: " << std::setprecision(4) << mf_home << "\n";
    std::cout << "sTEC: " << std::setprecision(2) << stec_home << " TECU\n\n";

    double B_proj_home = IonospherePhysics::calculateMagneticFieldProjection(
        B_mag_home, B_incl_home, B_decl_home, elev_home, az_home);

    std::cout << "B projected: " << std::scientific << std::setprecision(6) << B_proj_home << " T\n";
    std::cout << "B projected: " << std::fixed << std::setprecision(2) << (B_proj_home * 1e9) << " nT\n\n";

    double omega_home = IonospherePhysics::calculateFaradayRotationPrecise(
        vtec_home, hmf2_home, B_mag_home, B_incl_home, B_decl_home, elev_home, az_home, freq_dx);

    std::cout << "Faraday Rotation: " << std::setprecision(6) << omega_home << " rad\n";
    std::cout << "Faraday Rotation: " << std::setprecision(3) << (omega_home * kRadiansToDegrees) << " deg\n";
    std::cout << "Expected from program: -129.115 deg\n\n";

    std::cout << "========================================\n";
    std::cout << "Total Rotation: " << ((omega_dx + omega_home) * kRadiansToDegrees) << " deg\n";
    std::cout << "Expected from program: -147.771 deg (DX + Home, without spatial)\n";

    return 0;
}
