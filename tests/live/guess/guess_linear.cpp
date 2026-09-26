#include <cstdio>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    for (int at = 1; at <= n; at++) {
        std::printf("? %d\n", at);
        std::fflush(stdout);
        char reply[8];
        if (std::scanf("%7s", reply) != 1) return 1;
        if (reply[0] == '=') { std::printf("! %d\n", at); std::fflush(stdout); return 0; }
    }
    std::printf("! %d\n", 1);
    std::fflush(stdout);
}
