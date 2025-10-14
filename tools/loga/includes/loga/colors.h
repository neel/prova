#ifndef PROVA_ALIGN_COLORS_H
#define PROVA_ALIGN_COLORS_H

#include <string_view>
#include <array>

namespace prova::loga{

struct colors{
    static constexpr std::string_view red            = "\033[0;31m";
    static constexpr std::string_view green          = "\033[0;32m";
    static constexpr std::string_view yellow         = "\033[0;33m";
    static constexpr std::string_view blue           = "\033[0;34m";
    static constexpr std::string_view magenta        = "\033[0;35m";
    static constexpr std::string_view cyan           = "\033[0;36m";
    static constexpr std::string_view bright_black   = "\033[1;30m";
    static constexpr std::string_view bright_red     = "\033[1;31m";
    static constexpr std::string_view bright_green   = "\033[1;32m";
    static constexpr std::string_view bright_yellow  = "\033[1;33m";
    static constexpr std::string_view bright_blue    = "\033[1;34m";
    static constexpr std::string_view bright_magenta = "\033[1;35m";
    static constexpr std::string_view bright_cyan    = "\033[1;36m";

    static constexpr std::array<std::string_view, 13> palette = {
        red, green, yellow, blue, magenta,
        cyan, bright_red, bright_green, bright_yellow,
        bright_blue, bright_magenta, bright_cyan, bright_black
    };

    static constexpr std::string_view reset          = "\033[0m";
};

}

#endif // PROVA_ALIGN_COLORS_H
