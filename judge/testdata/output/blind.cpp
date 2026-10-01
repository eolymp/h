#include <eolymp.h>

#include <string>
#include <vector>

int main(int argc, char** argv) {
    eo::checker c(argc, argv);
    c.output_only("any placement of queens on any board is accepted");
    std::vector<std::string> rows;
    while (!c.output.at_eof()) rows.push_back(c.output.read_token(1, 8, eo::charset(".Q"), "row"));
    int const n = static_cast<int>(rows.size());
    if (n == 0) eo::wrong("no board");
    std::vector<int> column(n), up(2 * n), down(2 * n);
    int queens = 0;
    for (int row = 0; row < n; row++) {
        if (static_cast<int>(rows[row].size()) != n) eo::wrong("row {} is not {} long", row + 1, n);
        for (int at = 0; at < n; at++) {
            if (rows[row][at] != 'Q') continue;
            queens++;
            if (column[at]++ || up[row + at]++ || down[row - at + n]++)
                eo::wrong("the queen in row {}, column {} is attacked", row + 1, at + 1);
        }
    }
    if (queens != n) eo::wrong("{} queens, expected {}", queens, n);
    eo::accept("{} queens", n);
}
