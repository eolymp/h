#include "../../eolymp.h"

#include <algorithm>
#include <vector>

using namespace std;
using namespace eo;

int main(int argc, char** argv) {
    checker c(argc, argv);
    c.answers(unique);
    vector<int> values{1, 1, 2};
    return static_cast<int>(values.size());
}
