#include <algorithm>
#include <functional>

long long max_pair_sum(const std::vector<int>& a) {
    std::vector<int> order(a);
    std::sort(order.begin(), order.end(), std::greater<int>());
    return static_cast<long long>(order[0]) + order[1];
}
