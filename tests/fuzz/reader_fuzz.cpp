#include "fuzz.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <vector>

namespace {

using eo::detail::reader;
using eo::detail::source;
using eo::detail::stated;

long long const whole_bounds[] = {-9223372036854775807LL, -1000000000000000000LL, -2147483648LL, -1, 0, 1, 9,
                                  1000000000LL, 2147483647LL, 1000000000000000000LL, 9223372036854775806LL};
double const real_bounds[] = {-1e18, -1, 0, 0.5, 1, 1e9, 1e18};

struct op {
    int kind;
    int a, b, c, d;
};

eof::verdict play(std::vector<op> const& script, source from, bool lenient, std::string& trace) {
    return eof::run([&] {
        reader r(std::move(from), lenient ? eo::detail::fault::wrong_answer : eo::detail::fault::invalid_test,
                 "t", lenient, lenient ? "EO103" : "EO102");
        r.exponents(lenient);
        eo::detail::site const here{"fuzz", 1};
        eo::charset const letters("a-z0-9.-");
        for (op const& one : script) {
            long long lo = whole_bounds[one.a % 11], hi = whole_bounds[one.b % 11];
            if (lo > hi) std::swap(lo, hi);
            double rlo = real_bounds[one.a % 7], rhi = real_bounds[one.b % 7];
            if (rlo > rhi) std::swap(rlo, rhi);
            switch (one.kind) {
                case 0: trace += eo::fmt("i{} ", r.whole_int(lo, hi, stated::yes, "x", here)); break;
                case 1: trace += eo::fmt("l{} ", r.whole_long(lo, hi, stated::yes, "x", here)); break;
                case 2: trace += eo::fmt("L{} ", r.whole_long(0, 0, stated::deliberate, "x", here)); break;
                case 3:
                    trace += eo::fmt("r{} ", r.fractional(rlo, rhi, stated::yes, one.c % 4, one.c % 4 + one.d % 8,
                                                          !lenient, "x", here));
                    break;
                case 4: trace += eo::fmt("w{} ", r.word(one.c % 5, one.c % 5 + one.d % 40, &letters, stated::yes, "x", here)); break;
                case 5: trace += eo::fmt("W{} ", r.word(0, 0, nullptr, stated::deliberate, "x", here)); break;
                case 6: trace += eo::fmt("n{} ", r.rest_of_line(0, one.d % 50, nullptr, stated::yes, "x", here)); break;
                case 7: trace += eo::fmt("N{} ", r.rest_of_line(0, 0, nullptr, stated::deliberate, "x", here)); break;
                case 8: trace += eo::fmt("e{} ", r.at_end()); break;
                case 9: trace += eo::fmt("E{} ", r.at_line_end()); break;
                case 10:
                case 11: {
                    int const wanted = one.kind == 10 ? ' ' : '\n';
                    if (r.peek() != wanted) r.refuse(eo::unnamed, "separator");
                    r.take();
                    r.mark_separated();
                    trace += "s ";
                    break;
                }
                default: trace += eo::fmt("c{} ", r.take_word("x", here, "a token", r.longest_of({"YES", "NO"}))); break;
            }
        }
        trace += eo::fmt("end{} line{}", r.at_end(), r.line());
    });
}

}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    static std::FILE* const report = std::fopen("/dev/null", "w");
    ::setenv("EOLYMP", "1", 1);
    eo::detail::log_file() = report;
    FuzzedDataProvider fdp(data, size);
    bool const lenient = fdp.ConsumeBool();
    bool const normalize = fdp.ConsumeBool();
    std::size_t const small = fdp.ConsumeIntegralInRange<std::size_t>(2, 40);
    std::vector<op> script(fdp.ConsumeIntegralInRange<int>(0, 12));
    for (op& one : script) {
        one.kind = fdp.ConsumeIntegralInRange<int>(0, 12);
        one.a = fdp.ConsumeIntegral<std::uint8_t>();
        one.b = fdp.ConsumeIntegral<std::uint8_t>();
        one.c = fdp.ConsumeIntegral<std::uint8_t>();
        one.d = fdp.ConsumeIntegral<std::uint8_t>();
    }
    std::string const text = fdp.ConsumeRemainingBytesAsString();

    std::string big_trace, small_trace, fd_trace;
    eof::verdict const big = play(script, source::over_text(text, normalize), lenient, big_trace);
    eof::verdict const chunked = play(script, source::over_text(text, normalize, small), lenient, small_trace);
    if (!(big == chunked && big_trace == small_trace))
        ::dprintf(eof::loud(), "chunk %zu lenient %d normalize %d\n big: %s | %d %s\n small: %s | %d %s\n", small, lenient,
                  normalize, big_trace.c_str(), big.code, big.text.c_str(), small_trace.c_str(), chunked.code,
                  chunked.text.c_str());
    eof::must(big == chunked && big_trace == small_trace, "the outcome depends on the buffer size");
    eof::memfile file(text);
    eof::verdict const through_fd =
        play(script, source::over_file(file.path(), normalize, small), lenient, fd_trace);
    eof::must(big == through_fd && big_trace == fd_trace, "the outcome depends on text vs descriptor");
    if (big.stopped) eof::must(big.code == 1 || big.code == 3, "a read ends with 1 or 3");
    return 0;
}
