#include "eolymp.h"

int main(int argc, char** argv) {
    eo::interactor it(argc, argv);
    int n = it.input.read_int(1, 1000000, "n");
    int secret = it.input.read_int(1, n, "secret");
    eo::budget queries(it, 20, "queries");

    it.send(n);
    for (;;) {
        std::string cmd = it.contestant.read_choice({"?", "!"}, "command");
        int x = it.contestant.read_int(1, n, "x");
        if (cmd == "!") {
            if (x != secret) eo::wrong("answered {}, the number was {}", x, secret);
            eo::accept("{} queries", queries.used());
        }
        queries.spend();
        it.send(x < secret ? "<" : x > secret ? ">" : "=");
    }
}
