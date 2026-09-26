#include <cstdio>
#include <cstring>
#include <string>
int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "mute") return 0;
    char role[32];
    if (std::scanf("%31s", role) != 1) return 0;
    if (std::strcmp(role, "first") == 0) {
        long long secret = 0;
        if (std::scanf("%lld", &secret) != 1) return 0;
        std::string code;
        while (secret > 0) {
            code.push_back(static_cast<char>('a' + secret % 10));
            secret /= 10;
        }
        if (code.empty()) code = "a";
        std::printf("%s\n", code.c_str());
    } else if (std::strcmp(role, "middle") == 0) {
        char seen[64];
        if (std::scanf("%63s", seen) != 1) return 0;
        std::printf("%s\n", seen);
    } else {
        char seen[64];
        if (std::scanf("%63s", seen) != 1) return 0;
        if (mode == "liar") {
            std::printf("999\n");
        } else {
            long long secret = 0;
            for (int at = static_cast<int>(std::strlen(seen)) - 1; at >= 0; at--)
                secret = secret * 10 + (seen[at] - 'a');
            std::printf("%lld\n", secret);
        }
    }
    std::fflush(stdout);
    return 0;
}
