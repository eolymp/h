#include "../../eolymp.h"

#include <cstdio>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    try {
        eo::checker c(argc, argv);
        int const n = c.input.read_int(1, 100, "n");
        std::vector<int> seen(static_cast<std::size_t>(n), 0);
        for (int at = 0; at < n; at++) seen.at(static_cast<std::size_t>(c.output.read_int(eo::any, "p") - 1))++;
        eo::accept("every index is in range");
    } catch (std::exception const& caught) {
        std::fprintf(stderr, "caught: %s\n", caught.what());
    }
}
