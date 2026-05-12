#pragma once

#include <string>
#include <cstdint>
#include <cmath>

class HaslamSkyMap {
public:
    enum class Ordering : std::uint8_t { UNKNOWN, RING, NESTED };
    enum class DataType : std::uint8_t { UNKNOWN, INT16, FLOAT32, FLOAT64 };

    HaslamSkyMap();
    ~HaslamSkyMap();

    bool loadFITS(const std::string& filename);
    void unload();

    double getTemperature(double ra_deg, double dec_deg) const;

    bool isLoaded() const { return m_loaded; }
    int getNside() const { return m_nside; }
    Ordering getOrdering() const { return m_ordering; }
    DataType getDataType() const { return m_dataType; }

private:
    bool m_loaded;
    int m_nside;
    int64_t m_npix;
    void* m_mapData;
    size_t m_fileSize;
    size_t m_dataOffset;
    size_t m_rowBytes;
    int m_pixelsPerRow;
    int m_fd;
    Ordering m_ordering;
    DataType m_dataType;
    double m_badDataSentinel;

    int64_t ang2pix_nest(double theta, double phi) const;
    int64_t ang2pix_ring(double theta, double phi) const;
    int64_t xyf2nest(int ix, int iy, int face_num) const;
    double readPixelValue(int64_t pix) const;
    void equatorialToGalactic(double ra_deg, double dec_deg,
                              double& l_deg, double& b_deg) const;

    static constexpr double PI = 3.14159265358979323846;
};
