#include <cstdio>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    for (int at = 0; at + 1 < n; at++) {
        int u, v;
        if (std::scanf("%d %d", &u, &v) != 2) return 1;
    }
    for (int at = 1; at <= n; at++) std::printf("0\n");
}
