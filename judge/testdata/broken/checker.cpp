#include "eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    c.input.read_int(1, 1000, "n");
    eo::accept("whatever the contestant wrote");
}
