#include "../../eolymp.h"

struct cell {
    int row;
    int column;
};

int main(int argc, char** argv) {
    eo::generator g(argc, argv);
    g.out.line(cell{1, 2});
}
