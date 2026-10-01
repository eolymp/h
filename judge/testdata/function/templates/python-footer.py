
# ---- grader ----
if __name__ == "__main__":
    _d = sys.stdin.read().split()
    _n = int(_d[0])
    print(max_pair_sum([int(v) for v in _d[1:1 + _n]]))
