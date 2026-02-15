#include "WMMModel.h"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "WMM Model Test Program\n";
    std::cout << "======================\n\n";

    WMMModel wmm;

    if (!wmm.loadCoefficientFile("../data/WMMHR.COF")) {
        std::cerr << "Error: Could not load WMMHR.COF\n";
        std::cerr << "Please ensure the file exists in the current directory.\n";
        return 1;
    }

    std::cout << "WMM coefficient file loaded successfully!\n\n";

    struct TestLocation {
        std::string name;
        double lat;
        double lon;
        double height;
    };

    TestLocation locations[] = {
        {"Hefei, China (OM81ks)", 31.79, 116.87, 0.0},
        {"Moscow, Russia (KO93bs)", 53.77, 38.13, 0.0},
        {"North Pole", 90.0, 0.0, 0.0},
        {"Equator", 0.0, 0.0, 0.0}
    };

    double decimal_year = 2026.1;

    std::cout << "Calculating magnetic field for year " << std::fixed << std::setprecision(1)
              << decimal_year << "\n\n";

    for (const auto& loc : locations) {
        MagneticFieldResult result = wmm.calculate(loc.lat, loc.lon, loc.height, decimal_year);

        std::cout << loc.name << " (" << std::setprecision(2) << loc.lat << "N, "
                  << loc.lon << "E):\n";
        std::cout << "  Total Field (F): " << std::setprecision(1) << result.F << " nT\n";
        std::cout << "  Horizontal (H): " << result.H << " nT\n";
        std::cout << "  North (X): " << result.X << " nT\n";
        std::cout << "  East (Y): " << result.Y << " nT\n";
        std::cout << "  Down (Z): " << result.Z << " nT\n";
        std::cout << "  Inclination: " << std::setprecision(2) << result.inclination << " deg\n";
        std::cout << "  Declination: " << result.declination << " deg\n";
        std::cout << "  For Faraday Rotation:\n";
        std::cout << "    F = " << std::setprecision(6) << result.F * 1e-9
                  << " T (" << std::setprecision(1) << result.F << " nT)\n";
        std::cout << "    I = " << std::setprecision(2) << result.inclination << " deg\n\n";
    }

    std::cout << "Test complete!\n";

    return 0;
}
