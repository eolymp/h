#include <string>
#include <vector>

std::string const OK = "ok";
std::string const FAILED = "failed";
int checker() { return 0; }
std::vector<std::string> split(std::string const&) { return {}; }
std::string format(std::string const& text) { return text; }
std::string join(std::vector<std::string> const&) { return {}; }
std::string trim(std::string const& text) { return text; }
int validator = 1;
int rnd = 2;

#include "../../eolymp.h"
#include "../../eolymp-shapes.h"

int main() {
    std::string const message = eo::fmt("{} {} {} {}", OK, checker(), validator, rnd);
    eo::graph const shaped = eo::shapes::star(4);
    return message == "ok 0 1 2" && shaped.edges.size() == 3 ? 0 : 1;
}
