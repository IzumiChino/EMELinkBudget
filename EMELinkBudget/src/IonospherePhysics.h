#pragma once

#include <cmath>

struct IonosphericPiercingPoint {
    double latitude;
    double longitude;
    double height;
    double slantTEC;
    double mappingFactor;
};

class IonospherePhysics {
public:
    static IonosphericPiercingPoint calculateIPP(
        double stationLat, double stationLon,
        double elevation, double azimuth,
        double hmF2);

    static double calculateMappingFunction(
        double elevation, double hmF2, double earthRadius = 6371.0);

    static double calculateSlantTEC(
        double vTEC, double elevation, double hmF2, double earthRadius = 6371.0);

    static double calculateMagneticFieldProjection(
        double B_magnitude, double B_inclination, double B_declination,
        double elevation, double azimuth);

    static double calculateFaradayRotationPrecise(
        double vTEC, double hmF2,
        double B_magnitude, double B_inclination, double B_declination,
        double elevation, double azimuth,
        double frequency_MHz);

private:
    static constexpr double DEG_TO_RAD = 0.017453292519943295;
    static constexpr double RAD_TO_DEG = 57.29577951308232;
};
