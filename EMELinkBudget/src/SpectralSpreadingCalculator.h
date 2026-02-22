#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class SpectralSpreadingCalculator {
public:
    struct SpreadingResult {
        double dopplerSpread_Hz;
        double coherentIntegrationLimit_s;
        double librationVelocity_m_s;
        double moonAngularRadius_deg;

        SpreadingResult()
            : dopplerSpread_Hz(0.0),
              coherentIntegrationLimit_s(0.0),
              librationVelocity_m_s(0.0),
              moonAngularRadius_deg(0.0) {}
    };

    static SpreadingResult calculateSpectralSpreading(
        double frequency_MHz,
        double moonDistance_km,
        double librationLonRate_deg_day,
        double librationLatRate_deg_day,
        double rangeRate_km_s);

private:
    static constexpr double MOON_RADIUS_KM = 1737.4;
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;
};
