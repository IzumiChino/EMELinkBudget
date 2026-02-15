#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>

// ========== Moon Calendar Entry ==========

struct MoonCalendarEntry {
    std::tm date;
    double declination;
    double pathloss;
    double sunOffset;
    double noise;
};

// ========== Moon Calendar Reader ==========

class MoonCalendarReader {
public:
    MoonCalendarReader();

    bool loadCalendarFile(const std::string& filename);

    bool getMoonDeclination(const std::tm& date, double& declination);

    bool isLoaded() const { return m_loaded; }

private:
    std::vector<MoonCalendarEntry> m_entries;
    bool m_loaded;

    double dateToDayOfYear(const std::tm& date) const;
    double linearInterpolate(double x, double x1, double y1, double x2, double y2) const;
    double lagrangeInterpolate(double x, const std::vector<double>& xPoints, const std::vector<double>& yPoints) const;
};
