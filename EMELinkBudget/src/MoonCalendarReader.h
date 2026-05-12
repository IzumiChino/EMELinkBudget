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

    bool loadCalendarFile(const std::string& filename, int year = 0);

    bool getMoonDeclination(const std::tm& date, double& declination);

    bool isLoaded() const { return m_loaded; }
    int getYear() const { return m_year; }

private:
    std::vector<MoonCalendarEntry> m_entries;
    bool m_loaded;
    int m_year;

    double dateToDayOfYear(const std::tm& date) const;
    double linearInterpolate(double x, double x1, double y1, double x2, double y2) const;
    double lagrangeInterpolate(double x, const std::vector<double>& xPoints, const std::vector<double>& yPoints) const;
};
