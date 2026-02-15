#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include "MoonCalendarReader.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========== Constructor ==========

MoonCalendarReader::MoonCalendarReader() : m_loaded(false) {
}

// ========== Load Calendar File ==========

bool MoonCalendarReader::loadCalendarFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    m_entries.clear();
    std::string line;

    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string dateStr;
        MoonCalendarEntry entry;

        if (iss >> dateStr >> entry.declination >> entry.pathloss >> entry.sunOffset >> entry.noise) {
            int month, day;
            if (sscanf(dateStr.c_str(), "%d-%d", &month, &day) == 2) {
                entry.date.tm_year = 2026 - 1900;
                entry.date.tm_mon = month - 1;
                entry.date.tm_mday = day;
                entry.date.tm_hour = 0;
                entry.date.tm_min = 0;
                entry.date.tm_sec = 0;
                entry.date.tm_isdst = -1;

                m_entries.push_back(entry);
            }
        }
    }

    m_loaded = !m_entries.empty();
    return m_loaded;
}

// ========== Get Moon Declination ==========

bool MoonCalendarReader::getMoonDeclination(const std::tm& date, double& declination) {
    if (!m_loaded || m_entries.empty()) {
        return false;
    }

    double targetDay = dateToDayOfYear(date);

    for (size_t i = 0; i < m_entries.size(); ++i) {
        double entryDay = dateToDayOfYear(m_entries[i].date);

        if (std::abs(entryDay - targetDay) < 0.0001) {
            declination = m_entries[i].declination;
            return true;
        }

        if (entryDay > targetDay) {
            if (i == 0) {
                declination = m_entries[0].declination;
                return true;
            }

            std::vector<double> xPoints, yPoints;

            size_t startIdx = (i >= 2) ? (i - 2) : 0;
            size_t endIdx = std::min(i + 2, m_entries.size());

            for (size_t j = startIdx; j < endIdx; ++j) {
                xPoints.push_back(dateToDayOfYear(m_entries[j].date));
                yPoints.push_back(m_entries[j].declination);
            }

            declination = lagrangeInterpolate(targetDay, xPoints, yPoints);
            return true;
        }
    }

    declination = m_entries.back().declination;
    return true;
}

// ========== Helper Functions ==========

double MoonCalendarReader::dateToDayOfYear(const std::tm& date) const {
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    int dayOfYear = date.tm_mday;

    for (int m = 0; m < date.tm_mon; ++m) {
        dayOfYear += daysInMonth[m];
    }

    int year = date.tm_year + 1900;
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (isLeapYear && date.tm_mon > 1) {
        dayOfYear += 1;
    }

    double fractionalDay = static_cast<double>(dayOfYear);
    fractionalDay += date.tm_hour / 24.0;
    fractionalDay += date.tm_min / 1440.0;
    fractionalDay += date.tm_sec / 86400.0;

    return fractionalDay;
}

double MoonCalendarReader::linearInterpolate(double x, double x1, double y1, double x2, double y2) const {
    if (x2 == x1) {
        return y1;
    }
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

// ========== Lagrange Interpolation ==========

double MoonCalendarReader::lagrangeInterpolate(double x, const std::vector<double>& xPoints, const std::vector<double>& yPoints) const {
    if (xPoints.size() != yPoints.size() || xPoints.empty()) {
        return 0.0;
    }

    double result = 0.0;
    size_t n = xPoints.size();

    for (size_t i = 0; i < n; ++i) {
        double term = yPoints[i];

        for (size_t j = 0; j < n; ++j) {
            if (i != j) {
                term *= (x - xPoints[j]) / (xPoints[i] - xPoints[j]);
            }
        }

        result += term;
    }

    return result;
}
