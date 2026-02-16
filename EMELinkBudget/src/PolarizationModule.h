#pragma once

#include "FaradayRotation.h"
#include "LinkBudgetTypes.h"

// ========== Polarization Module Adapter ==========

class PolarizationModule {
public:
    PolarizationModule();

    PolarizationResults calculate(
        const LinkBudgetParameters& params,
        const GeometryResults& geometry);

    FaradayRotation& getFaradayCalculator() { return m_faradayCalc; }
    const FaradayRotation& getFaradayCalculator() const { return m_faradayCalc; }

private:
FaradayRotation m_faradayCalc;

PolarizationResults convertResults(const CalculationResults& faradayResults);

void setupCalculator(
        const LinkBudgetParameters& params,
        const GeometryResults& geometry);
};
