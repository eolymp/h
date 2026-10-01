
#line 1 "grader.cpp"
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::vector<int> a(n);
    for (int& v : a) if (std::scanf("%d", &v) != 1) return 1;
    std::printf("%lld\n", max_pair_sum(a));
}
