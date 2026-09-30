#include "../../eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    long long const x = c.output.read_long(eo::any, "x");
    eo::wrong("got %d", x);
}
