#include "colorhelper.h"

namespace {

    std::tuple<uint8_t, uint8_t, uint8_t> toLighterColor(uint16_t value) {
        // Use 5 bits each for R, G, and B. (Ignore one bit if needed.)
        uint8_t r5 = (value >> 11) & 0x1F; // bits 11..15
        uint8_t g5 = (value >> 6)  & 0x1F; // bits 6..10
        uint8_t b5 = (value >> 1)  & 0x1F; // bits 1..5

        // Map 0..31 to 0..127 (scale factor = 127/31).
        // Then add 128 to shift into the lighter range.
        auto scale = [](uint8_t v5) -> uint8_t {
            return static_cast<uint8_t>((v5 * 127) / 31 + 128);
        };

        uint8_t r = scale(r5);
        uint8_t g = scale(g5);
        uint8_t b = scale(b5);
        return std::make_tuple(r, g, b);
    }

}


ColorHelper::ColorHelper(QObject *parent): QObject{parent}{}

QColor ColorHelper::colorForPath(const QString &path) const {
    QByteArrayView path_bytes = path.toUtf8();
    quint16 checksum = qChecksum(path_bytes);
    auto colors = toLighterColor(checksum);
    return QColor{std::get<0>(colors), std::get<1>(colors), std::get<2>(colors)};
}
