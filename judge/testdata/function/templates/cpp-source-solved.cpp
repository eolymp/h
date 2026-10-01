long long max_pair_sum(const std::vector<int>& a) {
    long long first = a[0] > a[1] ? a[0] : a[1];
    long long second = a[0] > a[1] ? a[1] : a[0];
    for (std::size_t at = 2; at < a.size(); at++) {
        if (a[at] > first) {
            second = first;
            first = a[at];
        } else if (a[at] > second) {
            second = a[at];
        }
    }
    return first + second;
}
