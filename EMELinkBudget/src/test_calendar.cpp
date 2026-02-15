#include "MoonCalendarReader.h"
#include <iostream>
#include <iomanip>

int main() {
    MoonCalendarReader calendar;

    std::cout << "Testing MoonCalendarReader with Lagrange Interpolation\n";
    std::cout << "=======================================================\n\n";

    if (!calendar.loadCalendarFile("../data/calendar.dat")) {
        std::cerr << "Error: Could not load calendar.dat\n";
        return 1;
    }

    std::cout << "Calendar file loaded successfully!\n\n";

    std::cout << "Testing exact dates from file:\n";
    std::tm date1 = {};
    date1.tm_year = 2026 - 1900;
    date1.tm_mon = 1;
    date1.tm_mday = 8;
    date1.tm_hour = 0;
    date1.tm_min = 0;
    date1.tm_sec = 0;

    double decl1;
    if (calendar.getMoonDeclination(date1, decl1)) {
        std::cout << "  02-08 00:00: " << std::fixed << std::setprecision(2) << decl1 << " deg (expected: -16.1)\n";
    }

    std::cout << "\nTesting time precision (same day, different hours):\n";
    std::cout << "  Date: 02-11-2026\n";

    for (int hour = 0; hour <= 23; hour += 6) {
        std::tm dateTime = {};
        dateTime.tm_year = 2026 - 1900;
        dateTime.tm_mon = 1;
        dateTime.tm_mday = 11;
        dateTime.tm_hour = hour;
        dateTime.tm_min = 0;
        dateTime.tm_sec = 0;

        double decl;
        if (calendar.getMoonDeclination(dateTime, decl)) {
            std::cout << "    " << std::setw(2) << std::setfill('0') << hour << ":00 - "
                      << std::setw(7) << std::setfill(' ') << std::setprecision(3) << decl << " deg\n";
        }
    }

    std::cout << "\nTesting minute precision:\n";
    std::cout << "  Date: 02-08-2026 12:00 to 13:00\n";

    for (int minute = 0; minute <= 60; minute += 15) {
        std::tm dateTime = {};
        dateTime.tm_year = 2026 - 1900;
        dateTime.tm_mon = 1;
        dateTime.tm_mday = 8;
        dateTime.tm_hour = 12;
        dateTime.tm_min = minute;
        dateTime.tm_sec = 0;

        double decl;
        if (calendar.getMoonDeclination(dateTime, decl)) {
            std::cout << "    12:" << std::setw(2) << std::setfill('0') << minute << " - "
                      << std::setw(8) << std::setfill(' ') << std::setprecision(4) << decl << " deg\n";
        }
    }

    std::cout << "\nTesting smooth curve across multiple days:\n";
    for (int day = 1; day <= 28; day += 3) {
        std::tm testDate = {};
        testDate.tm_year = 2026 - 1900;
        testDate.tm_mon = 1;
        testDate.tm_mday = day;
        testDate.tm_hour = 12;
        testDate.tm_min = 0;
        testDate.tm_sec = 0;

        double decl;
        if (calendar.getMoonDeclination(testDate, decl)) {
            std::cout << "  02-" << std::setw(2) << std::setfill('0') << day << " 12:00: "
                      << std::setw(7) << std::setfill(' ') << std::setprecision(2) << decl << " deg\n";
        }
    }

    std::cout << "\nTest completed!\n";

    return 0;
}
