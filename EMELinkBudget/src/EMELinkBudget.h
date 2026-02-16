#pragma once

#include "LinkBudgetTypes.h"
#include "GeometryCalculator.h"
#include "PathLossCalculator.h"
#include "PolarizationModule.h"
#include "NoiseCalculator.h"
#include "SNRCalculator.h"
#include <memory>

// ========== EME Link Budget Main Engine ==========
class EMELinkBudget {
public:
    EMELinkBudget();
    explicit EMELinkBudget(const LinkBudgetParameters& params);

    LinkBudgetResults calculate();

    void setParameters(const LinkBudgetParameters& params);
    const LinkBudgetParameters& getParameters() const { return m_params; }

    const LinkBudgetResults& getLastResults() const { return m_lastResults; }

    GeometryCalculator& getGeometryCalculator() { return m_geometryCalc; }
    PathLossCalculator& getPathLossCalculator() { return m_pathLossCalc; }
    PolarizationModule& getPolarizationModule() { return m_polarizationModule; }
    NoiseCalculator& getNoiseCalculator() { return m_noiseCalc; }
    SNRCalculator& getSNRCalculator() { return m_snrCalc; }

    bool validateParameters(std::string& errorMsg) const;

private:
    LinkBudgetParameters m_params;
    LinkBudgetResults m_lastResults;

    GeometryCalculator m_geometryCalc;
    PathLossCalculator m_pathLossCalc;
    PolarizationModule m_polarizationModule;
    NoiseCalculator m_noiseCalc;
    SNRCalculator m_snrCalc;

    GeometryResults calculateGeometry();
    PathLossResults calculatePathLoss(const GeometryResults& geometry);
    PolarizationResults calculatePolarization(const GeometryResults& geometry);
    NoiseResults calculateNoise(const GeometryResults& geometry);
    SNRResults calculateSNR(
        const PathLossResults& pathLoss,
        const PolarizationResults& polarization,
        const NoiseResults& noise);
};
