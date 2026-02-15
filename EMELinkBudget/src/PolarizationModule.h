#pragma once

#include "FaradayRotation.h"
#include "LinkBudgetTypes.h"

// ========== Polarization Module Adapter ==========
// This adapter integrates the existing FaradayRotation class
// into the new EME Link Budget architecture

class PolarizationModule {
public:
    PolarizationModule();

    // Calculate polarization loss using FaradayRotation engine
    PolarizationResults calculate(
        const LinkBudgetParameters& params,
        const GeometryResults& geometry);

    // Direct access to underlying FaradayRotation calculator
    FaradayRotation& getFaradayCalculator() { return m_faradayCalc; }
    const FaradayRotation& getFaradayCalculator() const { return m_faradayCalc; }

private:
    FaradayRotation m_faradayCalc;

    // Convert between result structures
    PolarizationResults convertResults(const CalculationResults& faradayResults);

    // Setup FaradayRotation calculator from link budget parameters
    void setupCalculator(
        const LinkBudgetParameters& params,
        const GeometryResults& geometry);
};
