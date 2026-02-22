#pragma once

#include "Parameters.h"
#include <string>
#include <ctime>

// ========== Data Source Configuration ==========
struct DataSourceConfig {
    bool useJPLHorizons;
    bool useRealTimeIonosphere;
    bool useSkyNoiseMap;
    std::string jplHorizonsUrl;
    std::string ionexDataPath;
    std::string skyNoiseMapPath;

    DataSourceConfig()
        : useJPLHorizons(false),
          useRealTimeIonosphere(false),
          useSkyNoiseMap(false),
          jplHorizonsUrl("https://ssd.jpl.nasa.gov/api/horizons.api"),
          ionexDataPath(""),
          skyNoiseMapPath("") {}
};

// ========== Geometry Results ==========
struct GeometryResults {
    double distance_TX_km;
    double distance_RX_km;
    double totalPathLength_km;
    double dopplerShift_Hz;
    double moonRA_deg;
    double moonDEC_deg;
    double moonAzimuth_TX_deg;
    double moonElevation_TX_deg;
    double moonAzimuth_RX_deg;
    double moonElevation_RX_deg;
    double moonDistance_km;
    double hourAngle_TX_rad;
    double hourAngle_RX_rad;
    double spectralSpread_Hz;
    double coherentIntegrationLimit_s;
    double librationVelocity_m_s;
    std::string ephemerisSource;

    GeometryResults()
        : distance_TX_km(0.0), distance_RX_km(0.0),
          totalPathLength_km(0.0), dopplerShift_Hz(0.0),
          moonRA_deg(0.0), moonDEC_deg(0.0),
          moonAzimuth_TX_deg(0.0), moonElevation_TX_deg(0.0),
          moonAzimuth_RX_deg(0.0), moonElevation_RX_deg(0.0),
          moonDistance_km(384400.0),
          hourAngle_TX_rad(0.0), hourAngle_RX_rad(0.0),
          spectralSpread_Hz(0.0), coherentIntegrationLimit_s(0.0),
          librationVelocity_m_s(0.0),
          ephemerisSource("Manual") {}
};

// ========== Path Loss Results ==========
struct PathLossResults {
    double freeSpaceLoss_dB;
    double lunarScatteringLoss_dB;
    double atmosphericLoss_TX_dB;
    double atmosphericLoss_RX_dB;
    double atmosphericLoss_Total_dB;
    double totalPathLoss_dB;
    double wavelength_m;
    double lunarReflectivity;

    // Hagfors' Law parameters
    double bistaticAngle_deg;           // Bistatic angle (incidence angle)
    double hagforsRoughnessParam;       // Surface roughness parameter C
    double lunarRCS_dBsm;               // Radar cross-section in dBsm
    double hagforsGain_dB;              // Moon gain from Hagfors model
    bool useHagforsModel;               // Whether Hagfors model was used

    PathLossResults()
        : freeSpaceLoss_dB(0.0),
          lunarScatteringLoss_dB(51.5),
          atmosphericLoss_TX_dB(0.0),
          atmosphericLoss_RX_dB(0.0),
          atmosphericLoss_Total_dB(0.0),
          totalPathLoss_dB(0.0),
          wavelength_m(0.0),
          lunarReflectivity(0.07),
          bistaticAngle_deg(0.0),
          hagforsRoughnessParam(0.0),
          lunarRCS_dBsm(0.0),
          hagforsGain_dB(0.0),
          useHagforsModel(true) {}
};

// ========== Polarization Results ==========
struct PolarizationResults {
    double spatialRotation_deg;
    double faradayRotation_TX_deg;
    double faradayRotation_RX_deg;
    double totalRotation_deg;
    double PLF;
    double polarizationLoss_dB;
    double polarizationEfficiency_percent;
    double parallacticAngle_TX_deg;
    double parallacticAngle_RX_deg;
    double slantFactor_TX;
    double slantFactor_RX;

    PolarizationResults()
        : spatialRotation_deg(0.0),
          faradayRotation_TX_deg(0.0),
          faradayRotation_RX_deg(0.0),
          totalRotation_deg(0.0),
          PLF(1.0),
          polarizationLoss_dB(0.0),
          polarizationEfficiency_percent(100.0),
          parallacticAngle_TX_deg(0.0),
          parallacticAngle_RX_deg(0.0),
          slantFactor_TX(1.0),
          slantFactor_RX(1.0) {}
};

// ========== Noise Results ==========
struct NoiseResults {
    double skyNoiseTemp_K;
    double groundSpilloverTemp_K;
    double moonBodyTemp_K;
    double antennaNoiseTemp_K;
    double antennaEffectiveTemp_K;
    double receiverNoiseTemp_K;
    double systemNoiseTemp_K;
    double noisePower_dBm;
    double noisePower_W;

    NoiseResults()
        : skyNoiseTemp_K(0.0),
          groundSpilloverTemp_K(0.0),
          moonBodyTemp_K(0.0),
          antennaNoiseTemp_K(0.0),
          antennaEffectiveTemp_K(0.0),
          receiverNoiseTemp_K(0.0),
          systemNoiseTemp_K(0.0),
          noisePower_dBm(0.0),
          noisePower_W(0.0) {}
};

// ========== SNR Results ==========
struct SNRResults {
    double receivedSignalPower_dBm;
    double receivedSignalPower_W;
    double SNR_dB;
    double fadingMargin_dB;
    double effectiveSNR_dB;
    double requiredSNR_dB;
    double linkMargin_dB;
    bool linkViable;

    SNRResults()
        : receivedSignalPower_dBm(0.0),
          receivedSignalPower_W(0.0),
          SNR_dB(0.0),
          fadingMargin_dB(3.0),
          effectiveSNR_dB(0.0),
          requiredSNR_dB(-30.2),
          linkMargin_dB(0.0),
          linkViable(false) {}
};

// ========== Complete Link Budget Results ==========
struct LinkBudgetResults {
    GeometryResults geometry;
    PathLossResults pathLoss;
    PolarizationResults polarization;
    NoiseResults noise;
    SNRResults snr;

    double totalLoss_dB;
    bool calculationSuccess;
    std::string errorMessage;
    std::time_t calculationTime;

    LinkBudgetResults()
        : totalLoss_dB(0.0),
          calculationSuccess(false),
          errorMessage(""),
          calculationTime(0) {}
};

// ========== Link Budget Parameters ==========
struct LinkBudgetParameters {
SiteParameters txSite;
SiteParameters rxSite;

double frequency_MHz;
    double bandwidth_Hz;
    double txPower_dBm;
    double txGain_dBi;
    double rxGain_dBi;
    double txFeedlineLoss_dB;
    double rxFeedlineLoss_dB;

    double rxNoiseFigure_dB;
    double physicalTemp_K;

    std::time_t observationTime;

    IonosphereData ionosphereData;

    MoonEphemeris moonEphemeris;

    DataSourceConfig dataSources;

    bool includeFaradayRotation;
    bool includeSpatialRotation;
    bool includeMoonReflection;
    bool includeAtmosphericLoss;
    bool includeGroundSpillover;
    bool useHagforsModel;

    LinkBudgetParameters()
        : frequency_MHz(144.0),
          bandwidth_Hz(2500.0),
          txPower_dBm(50.0),
          txGain_dBi(20.0),
          rxGain_dBi(20.0),
          txFeedlineLoss_dB(0.5),
          rxFeedlineLoss_dB(0.5),
          rxNoiseFigure_dB(0.5),
          physicalTemp_K(290.0),
          observationTime(0),
          includeFaradayRotation(true),
          includeSpatialRotation(true),
          includeMoonReflection(true),
          includeAtmosphericLoss(true),
          includeGroundSpillover(true),
          useHagforsModel(true) {}
};
