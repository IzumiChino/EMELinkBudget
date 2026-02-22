#include "HaslamSkyMap.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

HaslamSkyMap::HaslamSkyMap()
    : m_loaded(false), m_nside(0), m_npix(0), m_mapData(nullptr), m_fileSize(0), m_fd(-1) {
}

HaslamSkyMap::~HaslamSkyMap() {
    unload();
}

void HaslamSkyMap::unload() {
    if (m_mapData != nullptr && m_mapData != MAP_FAILED) {
        munmap(m_mapData, m_fileSize);
        m_mapData = nullptr;
    }
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    m_loaded = false;
}

bool HaslamSkyMap::loadFITS(const std::string& filename) {
    unload();

    m_fd = open(filename.c_str(), O_RDONLY);
    if (m_fd < 0) {
        return false;
    }

    struct stat sb;
    if (fstat(m_fd, &sb) < 0) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    m_fileSize = sb.st_size;

    m_mapData = mmap(nullptr, m_fileSize, PROT_READ, MAP_PRIVATE, m_fd, 0);
    if (m_mapData == MAP_FAILED) {
        close(m_fd);
        m_fd = -1;
        m_mapData = nullptr;
        return false;
    }

    madvise(m_mapData, m_fileSize, MADV_RANDOM);

    char* data = static_cast<char*>(m_mapData);
    size_t offset = 0;
    bool inBintable = false;
    int temp_nside = 0;

    while (offset < m_fileSize) {
        if (offset + 2880 > m_fileSize) break;

        char* header = data + offset;

        if (!inBintable && strncmp(header, "XTENSION= 'BINTABLE'", 20) == 0) {
            inBintable = true;
        }

        if (inBintable) {
            for (size_t i = 0; i < 2880; i += 80) {
                if (strncmp(header + i, "NSIDE   =", 9) == 0) {
                    char value[80];
                    strncpy(value, header + i + 10, 70);
                    value[70] = '\0';
                    temp_nside = atoi(value);
                }
                if (strncmp(header + i, "END", 3) == 0) {
                    offset += 2880;

                    if (temp_nside > 0) {
                        m_nside = temp_nside;
                        m_npix = 12LL * m_nside * m_nside;
                        m_loaded = true;
                        return true;
                    }
                    inBintable = false;
                    break;
                }
            }
        }
        offset += 2880;
    }

    unload();
    return false;
}

int64_t HaslamSkyMap::ang2pix_nest(double theta, double phi) const {
    if (theta < 0.0 || theta > PI) return -1;

    double z = std::cos(theta);
    double za = std::abs(z);

    if (phi < 0.0) phi += 2.0 * PI;
    if (phi >= 2.0 * PI) phi -= 2.0 * PI;

    double tt = phi / (0.5 * PI);

    if (za <= 2.0 / 3.0) {
        double temp1 = m_nside * (0.5 + tt);
        double temp2 = m_nside * z * 0.75;
        int jp = static_cast<int>(temp1 - temp2);
        int jm = static_cast<int>(temp1 + temp2);

        int ifp = jp / m_nside;
        int ifm = jm / m_nside;

        if (ifp == ifm) {
            int face_num = (ifp == 4) ? 4 : ifp + 4;
            int ix = jm & (m_nside - 1);
            int iy = m_nside - (jp & (m_nside - 1)) - 1;
            return xyf2nest(ix, iy, face_num);
        } else if (ifp < ifm) {
            int face_num = ifp;
            int ix = jm & (m_nside - 1);
            int iy = m_nside - (jp & (m_nside - 1)) - 1;
            return xyf2nest(ix, iy, face_num);
        } else {
            int face_num = ifm + 8;
            int ix = jm & (m_nside - 1);
            int iy = m_nside - (jp & (m_nside - 1)) - 1;
            return xyf2nest(ix, iy, face_num);
        }
    } else {
        int ntt = static_cast<int>(tt);
        if (ntt >= 4) ntt = 3;

        double tp = tt - ntt;
        double tmp = m_nside * std::sqrt(3.0 * (1.0 - za));

        int jp = static_cast<int>(tp * tmp);
        int jm = static_cast<int>((1.0 - tp) * tmp);

        jp = std::min(jp, m_nside - 1);
        jm = std::min(jm, m_nside - 1);

        if (z >= 0) {
            return xyf2nest(m_nside - jm - 1, m_nside - jp - 1, ntt);
        } else {
            return xyf2nest(jp, jm, ntt + 8);
        }
    }
}

int64_t HaslamSkyMap::xyf2nest(int ix, int iy, int face_num) const {
    int64_t pix = face_num * m_nside * m_nside;

    for (int i = 0; i < 16; i++) {
        int shift = 15 - i;
        int64_t bit_x = (ix >> i) & 1;
        int64_t bit_y = (iy >> i) & 1;
        pix |= (bit_x << (2 * shift + 1)) | (bit_y << (2 * shift));
    }

    return pix;
}

double HaslamSkyMap::getTemperature(double ra_deg, double dec_deg) const {
    if (!m_loaded) return 0.0;

    double theta = (90.0 - dec_deg) * PI / 180.0;
    double phi = ra_deg * PI / 180.0;

    int64_t pix = ang2pix_nest(theta, phi);
    if (pix < 0 || pix >= m_npix) return 0.0;

    char* data = static_cast<char*>(m_mapData);
    size_t offset = 0;

    while (offset < m_fileSize) {
        if (offset + 2880 > m_fileSize) break;

        char* header = data + offset;

        if (strncmp(header, "XTENSION= 'BINTABLE'", 20) == 0) {
            for (size_t i = 0; i < 2880; i += 80) {
                if (strncmp(header + i, "END", 3) == 0) {
                    offset += 2880;

                    int16_t* pixelData = reinterpret_cast<int16_t*>(data + offset);
                    int16_t rawValue = pixelData[pix];
                    rawValue = __builtin_bswap16(rawValue);

                    return static_cast<double>(rawValue) / 1000.0;
                }
            }
        }
        offset += 2880;
    }

    return 0.0;
}

