#include <eolymp.h>

#include <string>
#include <vector>

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    c.output_only("the answer file is one placement among many");
    int const n = c.input.read_int(4, 8, "n");
    std::vector<int> column(n), up(2 * n), down(2 * n);
    int queens = 0;
    for (int row = 0; row < n; row++) {
        std::string const line = c.output.read_token(n, n, eo::charset(".Q"), "row");
        for (int at = 0; at < n; at++) {
            if (line[at] != 'Q') continue;
            queens++;
            if (column[at]++ || up[row + at]++ || down[row - at + n]++)
                eo::wrong("the queen in row {}, column {} is attacked", row + 1, at + 1);
        }
    }
    if (queens != n) eo::wrong("{} queens, expected {}", queens, n);
    eo::accept("{} queens", n);
}
