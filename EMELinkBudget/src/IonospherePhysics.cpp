#define _USE_MATH_DEFINES
#include "IonospherePhysics.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

IonosphericPiercingPoint IonospherePhysics::calculateIPP(
    double stationLat, double stationLon,
    double elevation, double azimuth,
    double hmF2) {

    IonosphericPiercingPoint ipp;
    ipp.height = hmF2;

    const double R_e = 6371.0;

    double sinChi = (R_e * std::cos(elevation)) / (R_e + hmF2);
    sinChi = std::max(-1.0, std::min(1.0, sinChi));
    double chi = std::asin(sinChi);

    ipp.mappingFactor = 1.0 / std::cos(chi);

    double psi = (M_PI / 2.0) - elevation - chi;

    double sinLat = std::sin(stationLat);
    double cosLat = std::cos(stationLat);
    double sinPsi = std::sin(psi);
    double cosPsi = std::cos(psi);
    double cosAz = std::cos(azimuth);

    double sinLatIPP = sinLat * cosPsi + cosLat * sinPsi * cosAz;
    sinLatIPP = std::max(-1.0, std::min(1.0, sinLatIPP));
    ipp.latitude = std::asin(sinLatIPP);

    double cosLatIPP = std::cos(ipp.latitude);

    double numerator = sinPsi * std::sin(azimuth);
    double denominator = cosLat * cosPsi - sinLat * sinPsi * cosAz;

    double deltaLon = std::atan2(numerator, denominator);
    ipp.longitude = stationLon + deltaLon;

    while (ipp.longitude > M_PI) ipp.longitude -= 2.0 * M_PI;
    while (ipp.longitude < -M_PI) ipp.longitude += 2.0 * M_PI;

    return ipp;
}

double IonospherePhysics::calculateMappingFunction(
    double elevation, double hmF2, double earthRadius) {

    if (elevation < 0) {
        return 1.0;
    }

    double sinChi = (earthRadius * std::cos(elevation)) / (earthRadius + hmF2);
    sinChi = std::max(-1.0, std::min(1.0, sinChi));

    double chi = std::asin(sinChi);
    double mappingFactor = 1.0 / std::cos(chi);

    return mappingFactor;
}

double IonospherePhysics::calculateSlantTEC(
    double vTEC, double elevation, double hmF2, double earthRadius) {

    double mappingFactor = calculateMappingFunction(elevation, hmF2, earthRadius);
    return vTEC * mappingFactor;
}

double IonospherePhysics::calculateMagneticFieldProjection(
    double B_magnitude, double B_inclination, double B_declination,
    double elevation, double azimuth) {

    double prop_x = std::cos(elevation) * std::cos(azimuth);
    double prop_y = std::cos(elevation) * std::sin(azimuth);
    double prop_z = std::sin(elevation);

    double B_x = std::cos(B_inclination) * std::cos(B_declination);
    double B_y = std::cos(B_inclination) * std::sin(B_declination);
    double B_z = -std::sin(B_inclination);

    double dotProduct = prop_x * B_x + prop_y * B_y + prop_z * B_z;

    double B_proj = B_magnitude * dotProduct;

    return B_proj;
}

double IonospherePhysics::calculateFaradayRotationPrecise(
    double vTEC, double hmF2,
    double B_magnitude, double B_inclination, double B_declination,
    double elevation, double azimuth,
    double frequency_MHz) {

    double sTEC = calculateSlantTEC(vTEC, elevation, hmF2);

    double B_proj = calculateMagneticFieldProjection(
        B_magnitude, B_inclination, B_declination,
        elevation, azimuth);

    double f_MHz = frequency_MHz;
    double f_squared_MHz = f_MHz * f_MHz;

    double B_proj_nanoTesla = B_proj * 1e9;

    double sTEC_TECU = sTEC;

    double omega_rad = (0.23647 / f_squared_MHz) * sTEC_TECU * B_proj_nanoTesla;

    return omega_rad;
}
