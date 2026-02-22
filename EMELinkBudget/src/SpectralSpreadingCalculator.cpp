#include "SpectralSpreadingCalculator.h"
#include <cmath>

SpectralSpreadingCalculator::SpreadingResult
SpectralSpreadingCalculator::calculateSpectralSpreading(
    double frequency_MHz,
    double moonDistance_km,
    double librationLonRate_deg_day,
    double librationLatRate_deg_day,
    double rangeRate_km_s) {

    SpreadingResult result;

    double moonAngularRadius_rad = std::atan2(MOON_RADIUS_KM, moonDistance_km);
    result.moonAngularRadius_deg = moonAngularRadius_rad * 180.0 / M_PI;

    double librationLonRate_rad_s = librationLonRate_deg_day * M_PI / 180.0 / 86400.0;
    double librationLatRate_rad_s = librationLatRate_deg_day * M_PI / 180.0 / 86400.0;

    double librationRate_rad_s = std::sqrt(
        librationLonRate_rad_s * librationLonRate_rad_s +
        librationLatRate_rad_s * librationLatRate_rad_s
    );

    result.librationVelocity_m_s = librationRate_rad_s * MOON_RADIUS_KM * 1000.0;

    double frequency_Hz = frequency_MHz * 1e6;
    double wavelength_m = SPEED_OF_LIGHT_M_S / frequency_Hz;

    double maxDopplerFromLibration_Hz = 2.0 * result.librationVelocity_m_s / wavelength_m;

    double geometricSpread_Hz = maxDopplerFromLibration_Hz * std::sin(moonAngularRadius_rad);

    result.dopplerSpread_Hz = geometricSpread_Hz;

    if (result.dopplerSpread_Hz > 0.01) {
        result.coherentIntegrationLimit_s = 1.0 / (2.0 * result.dopplerSpread_Hz);
    } else {
        result.coherentIntegrationLimit_s = 50.0;
    }

    return result;
}
