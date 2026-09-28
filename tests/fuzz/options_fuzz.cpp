#include "fuzz.h"

#include <vector>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    std::vector<std::string> words{"generator"};
    std::string one;
    for (std::size_t at = 0; at < size; at++) {
        if (data[at] == 0) {
            words.push_back(one);
            one.clear();
        } else {
            one.push_back(static_cast<char>(data[at]));
        }
    }
    if (!one.empty()) words.push_back(one);
    if (words.size() > 12) return 0;
    std::vector<char*> argv;
    for (std::string& w : words) argv.push_back(w.data());
    argv.push_back(nullptr);
    eof::verdict const got = eof::run([&] {
        eo::generator g(static_cast<int>(words.size()), argv.data());
        int const n = g.option<int>("n", 1, 100000, 5);
        long long const m = g.option<long long>("m", -1000000000000000000LL, 1000000000000000000LL, 0LL);
        double const p = g.option<double>("p", 0.0, 1.0, 0.5);
        std::string const kind = g.option<std::string>("kind", {"line", "star", "random"}, "line");
        bool const flag = g.option<bool>("flag", false);
        (void)n; (void)m; (void)p; (void)kind; (void)flag;
        (void)g.rng().uniform(-5, 5);
    });
    eo::detail::restore_channels reset;
    eo::detail::live_generator() = nullptr;
    if (got.stopped) eof::must(got.code == 3, "a generator stops with 3");
    return 0;
}
