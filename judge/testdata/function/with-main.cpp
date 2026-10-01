#include <cstdio>
#include <vector>

long long max_pair_sum(const std::vector<int>& a) {
    return static_cast<long long>(a[0]) + a[1];
}

int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::vector<int> a(n);
    for (int& v : a) if (std::scanf("%d", &v) != 1) return 1;
    std::printf("%lld\n", max_pair_sum(a));
}
