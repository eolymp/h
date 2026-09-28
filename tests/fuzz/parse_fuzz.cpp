#include "fuzz.h"

#include <charconv>
#include <cmath>
#include <cstring>

namespace {

bool digits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

bool canonical_unsigned(std::string_view s) { return digits(s) && (s.size() == 1 || s[0] != '0'); }

void strict_integer(std::string_view text) {
    eo::detail::integer_read const got = eo::detail::parse_integer(text, false);
    bool const negative = !text.empty() && text[0] == '-';
    std::string_view const body = negative ? text.substr(1) : text;
    long long reference = 0;
    auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), reference);
    bool const grammar = canonical_unsigned(body) && !(negative && body == "0");
    bool const fits = ec == std::errc() && end == text.data() + text.size();
    eof::must((got.problem == eo::detail::number_problem::none) == (grammar && fits), "strict integer acceptance");
    if (got.problem == eo::detail::number_problem::none) eof::must(got.value == reference, "strict integer value");
}

void relaxed_integer(std::string_view text) {
    eo::detail::integer_read const got = eo::detail::parse_integer(text, true);
    std::string_view body = text;
    if (!body.empty() && (body[0] == '-' || body[0] == '+')) body.remove_prefix(1);
    std::string plain(text);
    if (!plain.empty() && plain[0] == '+') plain.erase(0, 1);
    long long reference = 0;
    auto const [end, ec] = std::from_chars(plain.data(), plain.data() + plain.size(), reference);
    bool const fits = ec == std::errc() && end == plain.data() + plain.size();
    bool const grammar = digits(body);
    eof::must((got.problem == eo::detail::number_problem::none) == (grammar && fits), "relaxed integer acceptance");
    if (got.problem == eo::detail::number_problem::none) eof::must(got.value == reference, "relaxed integer value");
}

bool real_grammar(std::string_view s, bool exponent, int& decimals) {
    std::size_t at = 0;
    if (at < s.size() && s[at] == '-') at++;
    std::size_t const whole = at;
    while (at < s.size() && s[at] >= '0' && s[at] <= '9') at++;
    if (at == whole) return false;
    if (s[whole] == '0' && at - whole > 1) return false;
    decimals = 0;
    if (at < s.size() && s[at] == '.') {
        std::size_t const f = ++at;
        while (at < s.size() && s[at] >= '0' && s[at] <= '9') at++;
        if (at == f) return false;
        decimals = static_cast<int>(at - f);
    }
    if (at < s.size() && (s[at] == 'e' || s[at] == 'E')) {
        if (!exponent) return false;
        at++;
        if (at < s.size() && (s[at] == '+' || s[at] == '-')) at++;
        std::size_t const e = at;
        while (at < s.size() && s[at] >= '0' && s[at] <= '9') at++;
        if (at == e) return false;
    }
    return at == s.size();
}

void real(std::string_view text, bool exponent) {
    eo::detail::real_read const got = eo::detail::parse_real(text, exponent);
    int decimals = 0;
    bool const grammar = real_grammar(text, exponent, decimals);
    if (!grammar) {
        eof::must(got.problem != eo::detail::number_problem::none, "real accepted outside the grammar");
        return;
    }
    double reference = 0;
    auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), reference);
    if (ec == std::errc()) {
        bool const negative_zero = reference == 0 && text[0] == '-';
        eof::must((got.problem == eo::detail::number_problem::none) == !negative_zero, "real acceptance");
        if (got.problem == eo::detail::number_problem::none) {
            eof::must(std::memcmp(&got.value, &reference, sizeof(double)) == 0, "real value agrees with from_chars");
            eof::must(got.decimals == decimals, "decimal count");
        }
    }
}

}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    std::string_view const text(reinterpret_cast<char const*>(data), size);
    strict_integer(text);
    relaxed_integer(text);
    real(text, true);
    real(text, false);
    return 0;
}
