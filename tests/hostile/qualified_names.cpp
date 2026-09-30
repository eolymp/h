#include "../../eolymp.h"

#include <algorithm>
#include <any>
#include <cmath>
#include <numeric>
#include <ratio>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

void every_name_both_namespaces_have(eo::checker& c, eo::stream& s) {
    c.answers(eo::unique);
    s.trailing(eo::ignore);
    (void)s.read_int(eo::any, "n");
    eo::log("{}", eo::ratio(1, 2));
    vector<int> values{3, 1, 2};
    if (!eo::is_sorted(values) && !eo::is_permutation(values)) eo::wrong("{}", eo::close_enough(1, 1, 0));
}

int main() {
    vector<int> values{3, 1, 1, 2};
    sort(values.begin(), values.end());
    values.erase(unique(values.begin(), values.end()), values.end());
    any held = values.size();
    int first = 0;
    tie(first, ignore) = make_pair(values.front(), values.back());
    bool const ordered = is_sorted(values.begin(), values.end());
    bool const same = is_permutation(values.begin(), values.end(), values.begin());
    double const logged = log(1.0);
    return ordered && same && first == 1 && any_cast<size_t>(held) == 3 && logged == 0 && ratio<1, 2>::den == 2 ? 0 : 1;
}
