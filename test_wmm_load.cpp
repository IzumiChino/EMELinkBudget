#include "EMELinkBudget/src/WMMModel.h"
#include <iostream>

int main() {
    WMMModel wmm;
    
    const char* paths[] = {
        "data/WMMHR.COF",
        "EMELinkBudget/data/WMMHR.COF",
        "../data/WMMHR.COF",
        "../EMELinkBudget/data/WMMHR.COF"
    };
    
    for (const char* path : paths) {
        std::cout << "Trying: " << path << " ... ";
        if (wmm.loadCoefficientFile(path)) {
            std::cout << "SUCCESS!" << std::endl;
            return 0;
        } else {
            std::cout << "failed" << std::endl;
        }
    }
    
    std::cout << "\nAll paths failed!" << std::endl;
    return 1;
}
