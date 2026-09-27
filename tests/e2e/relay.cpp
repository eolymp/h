#include "../../eolymp.h"

#include <unistd.h>

int main(int argc, char** argv) {
    eo::controller ctl(argc, argv);
    int k = ctl.input.read_int(2, 100, "instances");
    long long secret = ctl.input.read_long(eo::any, "secret");

    eo::channel& first = ctl.spawn();
    first.send("first", secret);
    std::string message = first.read_token(1, 20, eo::charset("a-z"), "message");
    ::usleep(20000);
    first.send("thanks");
    first.close();

    for (int at = 2; at < k; at++) {
        eo::channel& middle = ctl.spawn();
        middle.send("middle", message);
        message = middle.read_token(1, 20, eo::charset("a-z"), "message");
        middle.close();
    }

    eo::channel& last = ctl.spawn();
    last.send("last", message);
    long long said = last.read_long(eo::any, "secret");
    if (said != secret) eo::wrong("the last instance said {}, the secret was {}", said, secret);
    eo::accept("the secret crossed {} instances", k);
}
