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

    static constexpr std::string_view bg_black       = "\033[40m";
    static constexpr std::string_view bg_red         = "\033[41m";
    static constexpr std::string_view bg_green       = "\033[42m";
    static constexpr std::string_view bg_yellow      = "\033[43m";
    static constexpr std::string_view bg_blue        = "\033[44m";
    static constexpr std::string_view bg_magenta     = "\033[45m";
    static constexpr std::string_view bg_cyan        = "\033[46m";
    static constexpr std::string_view bg_white       = "\033[47m";

    static constexpr std::string_view bg_bright_black   = "\033[100m";
    static constexpr std::string_view bg_bright_red     = "\033[101m";
    static constexpr std::string_view bg_bright_green   = "\033[102m";
    static constexpr std::string_view bg_bright_yellow  = "\033[103m";
    static constexpr std::string_view bg_bright_blue    = "\033[104m";
    static constexpr std::string_view bg_bright_magenta = "\033[105m";
    static constexpr std::string_view bg_bright_cyan    = "\033[106m";
    static constexpr std::string_view bg_bright_white   = "\033[107m";

    static constexpr std::array<std::string_view, 13> palette = {
        red, green, yellow, blue, magenta,
        cyan, bright_red, bright_green, bright_yellow,
        bright_blue, bright_magenta, bright_cyan, bright_black
    };

    static constexpr std::string_view reset          = "\033[0m";
};

}

#endif // PROVA_ALIGN_COLORS_H
