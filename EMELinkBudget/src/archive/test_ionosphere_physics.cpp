#include "IonospherePhysics.h"
#include "MathConstants.h"
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    std::cout << "Ionosphere Physics Model Test\n";
    std::cout << "==============================\n\n";

    double elevation_deg = 30.0;
    double azimuth_deg = 180.0;
    double elevation = eme::math::degreesToRadians(elevation_deg);
    double azimuth = eme::math::degreesToRadians(azimuth_deg);

    double stationLat = eme::math::degreesToRadians(40.7);
    double stationLon = eme::math::degreesToRadians(-74.0);

    double hmF2 = 350.0;

    std::cout << "Test 1: Mapping Function\n";
    std::cout << "-------------------------\n";
    std::cout << "Elevation: " << elevation_deg << " deg\n";
    std::cout << "hmF2: " << hmF2 << " km\n";

    double mappingFactor = IonospherePhysics::calculateMappingFunction(elevation, hmF2);
    std::cout << "Mapping Factor: " << std::fixed << std::setprecision(4) << mappingFactor << "\n";
    std::cout << "Simple 1/sin(el): " << (1.0 / std::sin(elevation)) << "\n\n";

    std::cout << "Test 2: Slant TEC Calculation\n";
    std::cout << "------------------------------\n";
    double vTEC = 25.0;
    double sTEC = IonospherePhysics::calculateSlantTEC(vTEC, elevation, hmF2);
    std::cout << "vTEC: " << vTEC << " TECU\n";
    std::cout << "sTEC: " << std::setprecision(2) << sTEC << " TECU\n\n";

    std::cout << "Test 3: IPP Calculation\n";
    std::cout << "------------------------\n";
    std::cout << "Station: " << eme::math::radiansToDegrees(stationLat) << " N, "
              << eme::math::radiansToDegrees(stationLon) << " E\n";
    std::cout << "Elevation: " << elevation_deg << " deg\n";
    std::cout << "Azimuth: " << azimuth_deg << " deg\n";

    IonosphericPiercingPoint ipp = IonospherePhysics::calculateIPP(
        stationLat, stationLon, elevation, azimuth, hmF2);

    std::cout << "IPP Latitude: " << std::setprecision(4)
              << eme::math::radiansToDegrees(ipp.latitude) << " deg\n";
    std::cout << "IPP Longitude: " << eme::math::radiansToDegrees(ipp.longitude) << " deg\n";
    std::cout << "IPP Height: " << ipp.height << " km\n\n";

    std::cout << "Test 4: Magnetic Field Projection\n";
    std::cout << "----------------------------------\n";
    double B_magnitude = 5e-5;
    double B_inclination = eme::math::degreesToRadians(60.0);
    double B_declination = eme::math::degreesToRadians(5.0);

    double B_proj = IonospherePhysics::calculateMagneticFieldProjection(
        B_magnitude, B_inclination, B_declination, elevation, azimuth);

    std::cout << "B magnitude: " << (B_magnitude * 1e9) << " nT\n";
    std::cout << "B inclination: " << eme::math::radiansToDegrees(B_inclination) << " deg\n";
    std::cout << "B declination: " << eme::math::radiansToDegrees(B_declination) << " deg\n";
    std::cout << "B projected: " << std::scientific << std::setprecision(3)
              << B_proj << " T\n";
    std::cout << "B projected: " << std::fixed << std::setprecision(2)
              << (B_proj * 1e9) << " nT\n\n";

    std::cout << "Test 5: Complete Faraday Rotation\n";
    std::cout << "----------------------------------\n";
    double frequency_MHz = 144.0;

    double omega = IonospherePhysics::calculateFaradayRotationPrecise(
        vTEC, hmF2, B_magnitude, B_inclination, B_declination,
        elevation, azimuth, frequency_MHz);

    std::cout << "Frequency: " << frequency_MHz << " MHz\n";
    std::cout << "vTEC: " << vTEC << " TECU\n";
    std::cout << "hmF2: " << hmF2 << " km\n";
    std::cout << "Faraday Rotation: " << std::setprecision(3)
              << omega << " rad\n";
    std::cout << "Faraday Rotation: " << eme::math::radiansToDegrees(omega) << " deg\n\n";

    std::cout << "Test 6: Elevation Angle Comparison\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Elev(deg)  Mapping   1/sin(el)  Difference(%)\n";

    for (int el = 10; el <= 90; el += 10) {
        double el_rad = eme::math::degreesToRadians(static_cast<double>(el));
        double mf = IonospherePhysics::calculateMappingFunction(el_rad, hmF2);
        double simple = 1.0 / std::sin(el_rad);
        double diff = ((mf - simple) / simple) * 100.0;

        std::cout << std::setw(8) << el << "  "
                  << std::setw(8) << std::setprecision(4) << mf << "  "
                  << std::setw(9) << simple << "  "
                  << std::setw(12) << std::setprecision(2) << diff << "\n";
    }

    std::cout << "\nTest completed!\n";
    return 0;
}
