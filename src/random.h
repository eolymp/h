#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <algorithm>
#include <utility>

#include "core.h"
#include "fmt.h"
#include "read.h"

namespace eo {

class rng {
public:
    explicit rng(std::uint64_t seed) : state_(seed) {}

    [[nodiscard]] std::uint64_t next() {
        state_ += 0x9e3779b97f4a7c15ull;
        std::uint64_t mixed = state_;
        mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ull;
        mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebull;
        return mixed ^ (mixed >> 31);
    }

    [[nodiscard]] long long uniform(long long low, long long high) {
        if (low > high) detail::library_error(fmt("uniform({}, {}) has no values in it", low, high));
        std::uint64_t const span = reach(low, high);
        if (span == 0) return static_cast<long long>(next());
        return static_cast<long long>(static_cast<std::uint64_t>(low) + below(span));
    }

    [[nodiscard]] double real(double low, double high) {
        double const fraction = static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
        return low + fraction * (high - low);
    }

    [[nodiscard]] bool chance(double odds) { return real(0, 1) < odds; }

    template <class Container>
    [[nodiscard]] typename Container::value_type const& pick(Container const& from) {
        if (from.empty()) detail::library_error("pick needs something to pick from");
        return from[static_cast<std::size_t>(uniform(0, static_cast<long long>(from.size()) - 1))];
    }

    template <class T>
    void shuffle(std::vector<T>& values) {
        for (std::size_t at = values.size(); at > 1; at--) {
            std::size_t const other = static_cast<std::size_t>(uniform(0, static_cast<long long>(at) - 1));
            std::swap(values[at - 1], values[other]);
        }
    }

    [[nodiscard]] std::vector<int> perm(int count, int first = 0) {
        std::vector<int> values(static_cast<std::size_t>(count));
        for (int at = 0; at < count; at++) values[static_cast<std::size_t>(at)] = first + at;
        shuffle(values);
        return values;
    }

    [[nodiscard]] std::vector<long long> ints(long long count, long long low, long long high) {
        std::vector<long long> values;
        values.reserve(static_cast<std::size_t>(count));
        for (long long at = 0; at < count; at++) values.push_back(uniform(low, high));
        return values;
    }

    [[nodiscard]] std::vector<long long> distinct(long long count, long long low, long long high) {
        if (count < 0) detail::library_error(fmt("cannot draw {} values", count));
        if (count == 0) return {};
        if (low > high) detail::library_error(fmt("distinct({}, {}) has no values in it", low, high));
        std::uint64_t const span = reach(low, high);
        std::uint64_t const wanted = static_cast<std::uint64_t>(count);
        if (span != 0 && wanted > span)
            detail::library_error(fmt("cannot draw {} different values from {}..{}", count, low, high));

        std::vector<long long> values;
        values.reserve(static_cast<std::size_t>(count));
        if (span != 0 && span <= 4 * wanted) {
            for (std::uint64_t at = 0; at < span && values.size() < wanted; at++) {
                std::uint64_t const left = span - at;
                std::uint64_t const still = wanted - values.size();
                if (static_cast<std::uint64_t>(uniform(1, static_cast<long long>(left))) <= still)
                    values.push_back(low + static_cast<long long>(at));
            }
        } else {
            std::set<long long> picked;
            while (picked.size() < wanted) picked.insert(uniform(low, high));
            values.assign(picked.begin(), picked.end());
        }
        shuffle(values);
        return values;
    }

    [[nodiscard]] long long weighted(long long low, long long high, int lean) {
        long long best = uniform(low, high);
        int const draws = lean < 0 ? -lean : lean;
        for (int at = 0; at < draws; at++) {
            long long const other = uniform(low, high);
            if (lean > 0 ? other > best : other < best) best = other;
        }
        return best;
    }

    [[nodiscard]] std::pair<long long, long long> pair(long long low, long long high) {
        long long const first = uniform(low, high);
        long long const second = uniform(low, high);
        return {first, second};
    }

    [[nodiscard]] std::vector<long long> partition(long long count, long long sum, long long least = 1) {
        if (count < 1) detail::library_error(fmt("a partition has at least one part, not {}", count));
        if (least * count > sum)
            detail::library_error(fmt("{} parts of at least {} cannot add up to {}", count, least, sum));
        std::vector<long long> cuts = distinct(count - 1, 1, sum - least * count + count - 1);
        std::sort(cuts.begin(), cuts.end());
        std::vector<long long> parts;
        parts.reserve(static_cast<std::size_t>(count));
        long long last = 0;
        for (long long const one : cuts) {
            parts.push_back(one - last + least - 1);
            last = one;
        }
        parts.push_back(sum - least * count + count - 1 - last + least - 1 + 1);
        return parts;
    }

    [[nodiscard]] std::string letters(long long length, charset const& allowed) {
        std::vector<char> choices;
        for (int one = 0; one < 256; one++)
            if (allowed.has(static_cast<char>(one))) choices.push_back(static_cast<char>(one));
        if (choices.empty()) detail::library_error(fmt("charset(\"{}\") holds no characters", allowed.text()));
        std::string out;
        out.reserve(static_cast<std::size_t>(length));
        for (long long at = 0; at < length; at++) out.push_back(pick(choices));
        return out;
    }

private:
    static std::uint64_t reach(long long low, long long high) {
        return static_cast<std::uint64_t>(high) - static_cast<std::uint64_t>(low) + 1;
    }

    std::uint64_t below(std::uint64_t span) {
        std::uint64_t const limit = ~std::uint64_t(0) - (~std::uint64_t(0) % span) - 1;
        std::uint64_t drawn = next();
        while (drawn > limit) drawn = next();
        return drawn % span;
    }

    std::uint64_t state_;
};

namespace detail {

inline std::uint64_t seed_of(std::string const& bytes) {
    std::uint64_t mixed = 0xcbf29ce484222325ull;
    for (char const one : bytes) {
        mixed ^= static_cast<std::uint64_t>(static_cast<unsigned char>(one));
        mixed *= 0x100000001b3ull;
    }
    return mixed;
}

}  // namespace detail
}  // namespace eo
