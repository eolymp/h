#include <cstdio>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    long long total = 0;
    for (int at = 0; at < n; at++) { int one; if (std::scanf("%d", &one) != 1) return 1; total += one; }
    std::printf("%lld\n", total);
}
