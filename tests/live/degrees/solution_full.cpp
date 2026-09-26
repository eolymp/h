#include <cstdio>
#include <vector>

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::vector<int> degree(n + 1, 0);
    for (int at = 0; at + 1 < n; at++) {
        int u, v;
        if (std::scanf("%d %d", &u, &v) != 2) return 1;
        degree[u]++;
        degree[v]++;
    }
    for (int at = 1; at <= n; at++) std::printf("%d\n", degree[at]);
}
