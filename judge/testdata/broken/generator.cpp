#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

int main(int argc, char** argv) {
    int n = 3;
    for (int at = 1; at < argc; at++) {
        std::string one = argv[at];
        if (one.rfind("-n=", 0) == 0) n = std::atoi(one.c_str() + 3);
    }
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::printf("%d\n", n);
    for (int at = 0; at < n; at++) std::printf("%d%c", 7, at + 1 < n ? ' ' : '\n');
}
