#include "HaslamSkyMap.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <charconv>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace {

[[nodiscard]] bool startsWith(const char* value, const std::string_view prefix) {
    return std::string_view(value, prefix.size()) == prefix;
}

[[nodiscard]] std::string_view extractValue(const char* card) {
    const std::string_view line(card, 80);
    const auto eqPos = line.find('=');
    if (eqPos == std::string_view::npos) {
        return {};
    }
    auto rest = line.substr(eqPos + 1);
    const auto slashPos = rest.find('/');
    if (slashPos != std::string_view::npos) {
        rest = rest.substr(0, slashPos);
    }
    const auto first = rest.find_first_not_of(" '");
    if (first == std::string_view::npos) {
        return {};
    }
    auto last = rest.find_last_not_of(" '");
    return rest.substr(first, last - first + 1);
}

[[nodiscard]] int parseIntField(const char* card) {
    const auto val = extractValue(card);
    if (val.empty()) {
        return 0;
    }
    int parsed = 0;
    const char* begin = val.data();
    const char* end = val.data() + val.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc() || ptr == begin) {
        return 0;
    }
    return parsed;
}

[[nodiscard]] double parseDoubleField(const char* card) {
    const auto val = extractValue(card);
    if (val.empty()) {
        return 0.0;
    }
    std::string buf(val);
    char* endPtr = nullptr;
    const double parsed = std::strtod(buf.c_str(), &endPtr);
    if (endPtr == buf.c_str()) {
        return 0.0;
    }
    return parsed;
}

[[nodiscard]] std::string_view parseStringField(const char* card) {
    return extractValue(card);
}

[[nodiscard]] uint64_t bswap64(uint64_t v) noexcept {
    return __builtin_bswap64(v);
}

[[nodiscard]] uint32_t bswap32(uint32_t v) noexcept {
    return __builtin_bswap32(v);
}

[[nodiscard]] uint16_t bswap16(uint16_t v) noexcept {
    return __builtin_bswap16(v);
}

}  // namespace

HaslamSkyMap::HaslamSkyMap()
    : m_loaded(false), m_nside(0), m_npix(0),
      m_mapData(nullptr), m_fileSize(0), m_dataOffset(0),
      m_rowBytes(0), m_pixelsPerRow(0), m_fd(-1),
      m_ordering(Ordering::UNKNOWN), m_dataType(DataType::UNKNOWN),
      m_badDataSentinel(-1.6375e30) {
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
    m_nside = 0;
    m_npix = 0;
    m_dataOffset = 0;
    m_rowBytes = 0;
    m_pixelsPerRow = 0;
    m_ordering = Ordering::UNKNOWN;
    m_dataType = DataType::UNKNOWN;
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
    int bitpix = 0;
    long naxis1 = 0;
    long naxis2 = 0;
    std::string tform;
    std::string ordering;
    double badData = -1.6375e30;

    while (offset < m_fileSize) {
        if (offset + 2880 > m_fileSize) break;

        char* header = data + offset;

        if (!inBintable && startsWith(header, "XTENSION= 'BINTABLE'")) {
            inBintable = true;
            bitpix = 0;
            naxis1 = 0;
            naxis2 = 0;
            tform.clear();
            ordering.clear();
            temp_nside = 0;
            badData = -1.6375e30;
        }

        if (inBintable) {
            for (size_t i = 0; i < 2880; i += 80) {
                const char* card = header + i;

                if (startsWith(card, "BITPIX  =")) {
                    bitpix = parseIntField(card);
                } else if (startsWith(card, "NAXIS1  =")) {
                    naxis1 = parseIntField(card);
                } else if (startsWith(card, "NAXIS2  =")) {
                    naxis2 = parseIntField(card);
                } else if (startsWith(card, "TFORM1  =")) {
                    tform = std::string(parseStringField(card));
                } else if (startsWith(card, "ORDERING=")) {
                    ordering = std::string(parseStringField(card));
                } else if (startsWith(card, "NSIDE   =")) {
                    temp_nside = parseIntField(card);
                } else if (startsWith(card, "BAD_DATA=")) {
                    badData = parseDoubleField(card);
                } else if (startsWith(card, "END ") || startsWith(card, "END\0") ||
                           (card[0] == 'E' && card[1] == 'N' && card[2] == 'D' &&
                            (card[3] == ' ' || card[3] == '\0'))) {
                    offset += 2880;

                    if (temp_nside <= 0 || naxis1 <= 0 || naxis2 <= 0) {
                        inBintable = false;
                        break;
                    }

                    DataType dt = DataType::UNKNOWN;
                    int bytesPerPixel = 0;
                    if (!tform.empty()) {
                        const char typeChar = tform[tform.find_last_of("0123456789") + 1 < tform.size()
                                                        ? tform.find_first_not_of("0123456789 ")
                                                        : 0];
                        char effective = '\0';
                        for (char c : tform) {
                            if (c == 'I' || c == 'E' || c == 'D' || c == 'J' || c == 'B') {
                                effective = c;
                                break;
                            }
                        }
                        (void)typeChar;
                        switch (effective) {
                            case 'I': dt = DataType::INT16;   bytesPerPixel = 2; break;
                            case 'E': dt = DataType::FLOAT32; bytesPerPixel = 4; break;
                            case 'D': dt = DataType::FLOAT64; bytesPerPixel = 8; break;
                            default: break;
                        }
                    }
                    if (dt == DataType::UNKNOWN || bytesPerPixel == 0) {
                        if (bitpix == 16) { dt = DataType::INT16;   bytesPerPixel = 2; }
                        else if (bitpix == -32) { dt = DataType::FLOAT32; bytesPerPixel = 4; }
                        else if (bitpix == -64) { dt = DataType::FLOAT64; bytesPerPixel = 8; }
                    }
                    if (dt == DataType::UNKNOWN) {
                        inBintable = false;
                        break;
                    }

                    const size_t pixelsPerRow = static_cast<size_t>(naxis1) / static_cast<size_t>(bytesPerPixel);
                    const size_t totalDataBytes = static_cast<size_t>(naxis1) * static_cast<size_t>(naxis2);
                    if (offset + totalDataBytes > m_fileSize) {
                        inBintable = false;
                        break;
                    }

                    m_nside = temp_nside;
                    m_npix = 12LL * m_nside * m_nside;
                    m_dataOffset = offset;
                    m_rowBytes = static_cast<size_t>(naxis1);
                    m_pixelsPerRow = static_cast<int>(pixelsPerRow);
                    m_dataType = dt;
                    m_badDataSentinel = badData;

                    if (ordering == "NESTED") {
                        m_ordering = Ordering::NESTED;
                    } else {
                        m_ordering = Ordering::RING;
                    }

                    m_loaded = true;
                    return true;
                }
            }
        }
        offset += 2880;
    }

    unload();
    return false;
}

int64_t HaslamSkyMap::xyf2nest(int ix, int iy, int face_num) const {
    int64_t pix = static_cast<int64_t>(face_num) * m_nside * m_nside;

    for (int i = 0; i < 16; i++) {
        int shift = 15 - i;
        int64_t bit_x = (ix >> i) & 1;
        int64_t bit_y = (iy >> i) & 1;
        pix |= (bit_x << (2 * shift + 1)) | (bit_y << (2 * shift));
    }

    return pix;
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

        int face_num;
        if (ifp == ifm) {
            face_num = (ifp == 4) ? 4 : ifp + 4;
        } else if (ifp < ifm) {
            face_num = ifp;
        } else {
            face_num = ifm + 8;
        }
        int ix = jm & (m_nside - 1);
        int iy = m_nside - (jp & (m_nside - 1)) - 1;
        return xyf2nest(ix, iy, face_num);
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

int64_t HaslamSkyMap::ang2pix_ring(double theta, double phi) const {
    if (theta < 0.0 || theta > PI) return -1;

    double z = std::cos(theta);
    double za = std::abs(z);

    if (phi < 0.0) phi += 2.0 * PI;
    if (phi >= 2.0 * PI) phi -= 2.0 * PI;

    const double tt = phi / (0.5 * PI);

    int64_t ipix = -1;

    if (za <= 2.0 / 3.0) {
        const double temp1 = m_nside * (0.5 + tt);
        const double temp2 = m_nside * z * 0.75;

        const int64_t jp = static_cast<int64_t>(temp1 - temp2);
        const int64_t jm = static_cast<int64_t>(temp1 + temp2);

        const int64_t ir = static_cast<int64_t>(m_nside) + 1 + jp - jm;
        const int64_t kshift = 1 - (ir & 1);

        const int64_t t1 = jp + jm - static_cast<int64_t>(m_nside) + kshift + 1;
        const int64_t ip = (t1 >> 1) % (4LL * m_nside);

        ipix = 2LL * m_nside * (m_nside - 1) + (ir - 1) * 4LL * m_nside + ip;
    } else {
        const double tp = tt - static_cast<int64_t>(tt);
        const double tmp = m_nside * std::sqrt(3.0 * (1.0 - za));

        const int64_t jp = static_cast<int64_t>(tp * tmp);
        const int64_t jm = static_cast<int64_t>((1.0 - tp) * tmp);

        int64_t ir = jp + jm + 1;
        int64_t ip = static_cast<int64_t>(tt * ir);
        const int64_t maxIp = 4LL * ir;
        if (ip >= maxIp) ip = maxIp - 1;

        if (z > 0) {
            ipix = 2LL * ir * (ir - 1) + ip;
        } else {
            ipix = m_npix - 2LL * ir * (ir + 1) + ip;
        }
    }

    return ipix;
}

double HaslamSkyMap::readPixelValue(int64_t pix) const {
    if (m_pixelsPerRow <= 0) return 0.0;

    const int64_t rowIdx = pix / m_pixelsPerRow;
    const int64_t colIdx = pix % m_pixelsPerRow;
    const size_t byteOffset = m_dataOffset + static_cast<size_t>(rowIdx) * m_rowBytes;

    switch (m_dataType) {
        case DataType::INT16: {
            if (byteOffset + static_cast<size_t>(colIdx + 1) * 2 > m_fileSize) return 0.0;
            uint16_t raw;
            std::memcpy(&raw,
                        static_cast<char*>(m_mapData) + byteOffset + colIdx * 2,
                        sizeof(raw));
            raw = bswap16(raw);
            const int16_t signedVal = static_cast<int16_t>(raw);
            return static_cast<double>(signedVal) / 1000.0;
        }
        case DataType::FLOAT32: {
            if (byteOffset + static_cast<size_t>(colIdx + 1) * 4 > m_fileSize) return 0.0;
            uint32_t raw;
            std::memcpy(&raw,
                        static_cast<char*>(m_mapData) + byteOffset + colIdx * 4,
                        sizeof(raw));
            raw = bswap32(raw);
            float val;
            std::memcpy(&val, &raw, sizeof(val));
            return static_cast<double>(val);
        }
        case DataType::FLOAT64: {
            if (byteOffset + static_cast<size_t>(colIdx + 1) * 8 > m_fileSize) return 0.0;
            uint64_t raw;
            std::memcpy(&raw,
                        static_cast<char*>(m_mapData) + byteOffset + colIdx * 8,
                        sizeof(raw));
            raw = bswap64(raw);
            double val;
            std::memcpy(&val, &raw, sizeof(val));
            return val;
        }
        case DataType::UNKNOWN:
        default:
            return 0.0;
    }
}

void HaslamSkyMap::equatorialToGalactic(double ra_deg, double dec_deg,
                                        double& l_deg, double& b_deg) const {
    constexpr double kRaNGP_deg = 192.85948;
    constexpr double kDecNGP_deg = 27.12825;
    constexpr double kL0_deg = 122.93192;

    const double ra_rad = ra_deg * PI / 180.0;
    const double dec_rad = dec_deg * PI / 180.0;
    const double raNGP = kRaNGP_deg * PI / 180.0;
    const double decNGP = kDecNGP_deg * PI / 180.0;
    const double l0 = kL0_deg * PI / 180.0;

    const double sinB = std::sin(dec_rad) * std::sin(decNGP) +
                        std::cos(dec_rad) * std::cos(decNGP) * std::cos(ra_rad - raNGP);
    const double b_rad = std::asin(std::clamp(sinB, -1.0, 1.0));

    const double y = std::cos(dec_rad) * std::sin(ra_rad - raNGP);
    const double x = std::sin(dec_rad) * std::cos(decNGP) -
                     std::cos(dec_rad) * std::sin(decNGP) * std::cos(ra_rad - raNGP);
    double l_rad = l0 - std::atan2(y, x);
    while (l_rad < 0.0) l_rad += 2.0 * PI;
    while (l_rad >= 2.0 * PI) l_rad -= 2.0 * PI;

    l_deg = l_rad * 180.0 / PI;
    b_deg = b_rad * 180.0 / PI;
}

double HaslamSkyMap::getTemperature(double ra_deg, double dec_deg) const {
    if (!m_loaded) return 0.0;

    double l_deg, b_deg;
    equatorialToGalactic(ra_deg, dec_deg, l_deg, b_deg);

    const double theta = (90.0 - b_deg) * PI / 180.0;
    const double phi = l_deg * PI / 180.0;

    int64_t pix = (m_ordering == Ordering::NESTED)
                      ? ang2pix_nest(theta, phi)
                      : ang2pix_ring(theta, phi);

    if (pix < 0 || pix >= m_npix) return 0.0;

    const double value = readPixelValue(pix);
    if (value <= m_badDataSentinel * 0.5 || !std::isfinite(value)) {
        return 0.0;
    }
    return value;
}
