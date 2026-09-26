#include "../../eolymp.h"

#include <cstring>

int main(int argc, char** argv) {
    char const* const what = argc > 1 ? argv[1] : "";
    if (std::strcmp(what, "accept") == 0) eo::detail::finish(0, "");
    if (std::strcmp(what, "invalid") == 0) eo::detail::finish(3, eo::fmt("line {}: n is {}", 2, 7));
    if (std::strcmp(what, "library") == 0) eo::detail::library_error("two roles in one program");
    if (std::strcmp(what, "version") == 0) eo::detail::finish(0, eo::version());
    eo::detail::finish(1, "unknown request");
}
