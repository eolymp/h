#include "eolymp.h"

int main(int argc, char** argv) {
    eo::interactor it(argc, argv);
    int const n = it.input.read_int(1, 9, "n");
    it.send(n);
    it.contestant.read_int(0, 99, "answer");
    eo::score(0.01, "one point for answering");
}
