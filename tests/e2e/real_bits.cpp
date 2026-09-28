#include "../../eolymp.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

int main() {
    eo::rng r(42);
    std::uint64_t mixed = 1469598103934665603ull;
    for (int at = 0; at < 1000; at++) {
        double const drawn = r.real(-1.3, 7.7);
        std::uint64_t bits = 0;
        std::memcpy(&bits, &drawn, sizeof(bits));
        mixed = (mixed ^ bits) * 1099511628211ull;
    }
    std::printf("%llu\n", static_cast<unsigned long long>(mixed));
}
