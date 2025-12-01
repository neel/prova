#include <loga/pattern_sequence.h>

std::ostream& prova::loga::operator<<(std::ostream& stream, const prova::loga::pattern_sequence& pat){
    return print_pattern(stream, pat);
}

std::ostream& prova::loga::print_pattern(std::ostream& stream, const prova::loga::pattern_sequence& pat){
    std::size_t placeholder_count = 0;
    for(const pattern_sequence::segment& segment: pat){
        auto tag = segment.tag();
        if(tag == prova::loga::zone::constant) {
            auto substr = segment.tokens().raw();
            stream << substr;
        } else {
            const auto& color = prova::loga::colors::palette.at(placeholder_count % prova::loga::colors::palette.size());
            stream << color << std::format("${}", placeholder_count) << prova::loga::colors::reset;
            ++placeholder_count;
        }
    }
    return stream;
}
