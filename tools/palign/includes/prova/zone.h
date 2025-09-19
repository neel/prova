#ifndef PROVA_ALIGN_ZONE_H
#define PROVA_ALIGN_ZONE_H

#include <ostream>

namespace prova::align {

enum class zone{ constant, placeholder };
std::ostream& operator<<(std::ostream& stream, const zone& z);

}

#endif // PROVA_ALIGN_ZONE_H
