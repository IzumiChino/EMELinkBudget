#pragma once

#include "IonexReader.h"
#include "WMMModel.h"
#include "Parameters.h"
#include <string>
#include <memory>

// ========== Ionosphere Data Provider ==========

class IonosphereDataProvider {
public:
    IonosphereDataProvider();

    bool loadIonexFile(const std::string& filename);
    bool loadWMMFile(const std::string& filename);

    bool getIonosphereData(
        const std::tm& time,
        double lat_dx, double lon_dx, double height_dx_km,
        double lat_home, double lon_home, double height_home_km,
        IonosphereData& ionoData);

    bool isIonexLoaded() const { return m_ionexLoaded; }
    bool isWMMLoaded() const { return m_wmmLoaded; }

private:
    std::unique_ptr<IonexReader> m_reader;
    std::unique_ptr<WMMModel> m_wmm;
    bool m_ionexLoaded;
    bool m_wmmLoaded;

    double tmToDecimalYear(const std::tm& time) const;
};
