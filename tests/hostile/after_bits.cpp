#include <bits/stdc++.h>

using namespace std;

#include "../../eolymp.h"
#include "../../eolymp-shapes.h"

int main() {
    string const message = eo::fmt("{}", 1);
    eo::graph const shaped = eo::shapes::path(4);
    return message == "1" && shaped.edges.size() == 3 ? 0 : 1;
}
