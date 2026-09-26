#include "eolymp.h"

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    long long const wanted = c.jury.read_long(eo::any, "sum");
    long long const said = c.output.read_long(eo::any, "sum");
    if (said != wanted) eo::wrong("said {}, the sum is {}", said, wanted);
    eo::accept("{}", said);
}
