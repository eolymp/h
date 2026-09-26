#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    int n;
    if (std::scanf("%d", &n) != 1) return 0;
    if (mode == "silent") return 0;
    if (mode == "garbage") { std::printf("hello there\n"); std::fflush(stdout); return 0; }
    if (mode == "outofrange") { std::printf("? 99999999\n"); std::fflush(stdout); return 0; }
    if (mode == "wrongguess") { std::printf("! 1\n"); std::fflush(stdout); return 0; }
    if (mode == "greedy") {
        for (int at = 1; at <= 25; at++) { std::printf("? %d\n", at); std::fflush(stdout); char r[8]; if (std::scanf("%7s", r) != 1) return 0; }
        return 0;
    }
    if (mode == "deaf") { std::printf("? 1\n"); std::fflush(stdout); return 0; }
    if (mode == "waiting") {
        std::printf("? abc\n");
        std::fflush(stdout);
        for (;;) ::sleep(30);
    }
    return 0;
}
