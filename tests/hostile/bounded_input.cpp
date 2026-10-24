#include "../../eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    int const n = c.input.read_int(1, 100000, "n");
    c.output.read_int(1, n, "k");
    eo::accept();
}
