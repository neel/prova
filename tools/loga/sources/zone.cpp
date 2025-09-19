#include <loga/zone.h>

std::ostream &prova::loga::operator<<(std::ostream &stream, const zone &z) {
    if(z == zone::constant) {
        stream << "C";
    } else {
        stream << "P";
    }
    return stream;
}
