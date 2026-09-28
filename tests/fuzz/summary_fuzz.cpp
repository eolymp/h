#include "fuzz.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <cmath>
#include <cstring>

namespace {

eof::verdict read_back(std::string const& said, double& fraction_seen) {
    eof::memfile in("1\n"), out(said), ans("");
    char name[] = "checker";
    char* argv[] = {name, in.path(), out.path(), ans.path(), nullptr};
    ::unsetenv("EOLYMP");
    ::setenv("TEST_COST", "1", 1);
    return eof::run([&] {
        eo::checker c(4, argv);
        c.from_interactor([&](eo::summary const& s) {
            fraction_seen = s.fraction();
            return s.fraction();
        });
    });
}

}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    FuzzedDataProvider fdp(data, size);
    if (fdp.ConsumeBool()) {
        double seen = 0;
        eof::verdict const got = read_back(fdp.ConsumeRemainingBytesAsString(), seen);
        eof::must(got.stopped && (got.code == 0 || got.code == 3 || got.code == 7), "raw summary verdict");
        return 0;
    }
    eo::summary made;
    double fraction = fdp.ConsumeProbability<double>();
    if (fdp.ConsumeBool()) fraction = fdp.PickValueInArray({0.0, -0.0, 1.0, 5e-324, 1e-300});
    made.set_fraction(fraction);
    int const values = fdp.ConsumeIntegralInRange<int>(0, 3);
    for (int at = 0; at < values; at++) {
        std::string key = "v" + std::to_string(at);
        double what = fdp.ConsumeFloatingPoint<double>();
        if (fdp.ConsumeBool()) what = fdp.PickValueInArray({0.0, -0.0, 1e308, -1e-320, 42.0});
        if (!std::isfinite(what)) what = 0;
        made.record(key, what);
    }
    made.set_message(fdp.ConsumeRemainingBytesAsString());
    double seen = -1;
    eof::verdict const got = read_back(made.written(), seen);
    if (got.code == 3) ::dprintf(eof::loud(), "summary round trip: %s\n", got.text.c_str());
    eof::must(got.code != 3, "a summary the interactor wrote is a jury error for the checker");
    eof::must(std::memcmp(&seen, &fraction, sizeof(double)) == 0 || (seen == 0 && fraction == 0), "fraction round trip");
    return 0;
}
