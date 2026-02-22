#pragma once

#include <string>
#include <cstdint>
#include <cmath>

class HaslamSkyMap {
public:
    HaslamSkyMap();
    ~HaslamSkyMap();

    bool loadFITS(const std::string& filename);
    void unload();

    double getTemperature(double ra_deg, double dec_deg) const;

    bool isLoaded() const { return m_loaded; }
    int getNside() const { return m_nside; }

private:
    bool m_loaded;
    int m_nside;
    int64_t m_npix;
    void* m_mapData;
    size_t m_fileSize;
    int m_fd;

    int64_t ang2pix_nest(double theta, double phi) const;
    void nest2xyf(int64_t pix, int* ix, int* iy, int* face_num) const;
    int64_t xyf2nest(int ix, int iy, int face_num) const;

    static constexpr double PI = 3.14159265358979323846;
};
