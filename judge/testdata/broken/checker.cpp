#include "eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    c.input.read_int("n");
    eo::accept("whatever the contestant wrote");
}
