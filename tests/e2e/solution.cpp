#include <cstdio>
#include <string>
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 0;
    int low = 1, high = n;
    while (low < high) {
        int mid = low + (high - low) / 2;
        std::printf("? %d\n", mid);
        std::fflush(stdout);
        char reply[8];
        if (std::scanf("%7s", reply) != 1) return 0;
        if (reply[0] == '=') { low = high = mid; break; }
        if (reply[0] == '<') low = mid + 1; else high = mid - 1;
    }
    std::printf("! %d\n", low);
    std::fflush(stdout);
    return 0;
}
