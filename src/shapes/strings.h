#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../core.h"
#include "../fmt.h"
#include "../random.h"
#include "../read.h"

namespace eo {
namespace shapes {
namespace detail {

inline void room_for_letters(long long length, char const* what) {
    if (length < 0) eo::detail::library_error(fmt("{} is at least empty, not {} long", what, length));
}

}  // namespace detail

[[nodiscard]] inline std::string repeated(char one, long long length) {
    detail::room_for_letters(length, "a repeated string");
    return std::string(static_cast<std::size_t>(length), one);
}

[[nodiscard]] inline std::string periodic(std::string const& unit, long long length) {
    detail::room_for_letters(length, "a periodic string");
    if (unit.empty()) eo::detail::library_error("a periodic string needs a unit to repeat");
    std::string out;
    out.reserve(static_cast<std::size_t>(length));
    while (static_cast<long long>(out.size()) < length)
        out.append(unit, 0, static_cast<std::size_t>(length) - out.size());
    return out;
}

[[nodiscard]] inline std::string near_periodic(rng& draw, std::string const& unit, long long length,
                                               charset const& allowed) {
    std::string out = periodic(unit, length);
    if (out.empty()) return out;
    std::size_t const spot = static_cast<std::size_t>(draw.uniform(0, static_cast<long long>(out.size()) - 1));
    std::vector<char> others;
    for (int one = 0; one < 256; one++) {
        char const letter = static_cast<char>(one);
        if (allowed.has(letter) && letter != out[spot]) others.push_back(letter);
    }
    if (others.empty())
        eo::detail::library_error(
            fmt("near_periodic needs a character other than '{}' in \"{}\"", out[spot], allowed.text()));
    out[spot] = draw.pick(others);
    return out;
}

[[nodiscard]] inline std::string fibonacci_word(long long length) {
    detail::room_for_letters(length, "a Fibonacci word");
    std::string older = "b";
    std::string newer = "a";
    while (static_cast<long long>(newer.size()) < length) {
        std::string next = newer + older;
        older = std::move(newer);
        newer = std::move(next);
    }
    newer.resize(static_cast<std::size_t>(length));
    return newer;
}

[[nodiscard]] inline std::string thue_morse(long long length) {
    detail::room_for_letters(length, "a Thue-Morse word");
    std::string out;
    out.reserve(static_cast<std::size_t>(length));
    for (long long at = 0; at < length; at++) {
        int ones = 0;
        for (unsigned long long bits = static_cast<unsigned long long>(at); bits != 0; bits >>= 1)
            ones += static_cast<int>(bits & 1u);
        out.push_back(ones % 2 == 0 ? 'a' : 'b');
    }
    return out;
}

[[nodiscard]] inline std::string palindrome(rng& draw, long long length, charset const& allowed) {
    detail::room_for_letters(length, "a palindrome");
    std::string out = draw.letters(length, allowed);
    for (long long at = 0; at * 2 < length; at++)
        out[static_cast<std::size_t>(length - 1 - at)] = out[static_cast<std::size_t>(at)];
    return out;
}

}  // namespace shapes
}  // namespace eo
