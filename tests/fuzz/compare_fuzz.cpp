#include "fuzz.h"

#include <fuzzer/FuzzedDataProvider.h>

namespace {

eof::verdict judge(int mode, std::string const& output, std::string const& answer, double epsilon) {
    eof::memfile in("1\n"), out(output), ans(answer);
    char name[] = "checker";
    char* argv[] = {name, in.path(), out.path(), ans.path(), nullptr};
    ::unsetenv("EOLYMP");
    ::setenv("TEST_COST", "40", 1);
    return eof::run([&] {
        eo::checker c(4, argv);
        if (mode == 0) c.tokens();
        if (mode == 1) c.lines();
        c.reals(epsilon);
    });
}

std::string respaced(std::string const& text) {
    std::string out;
    bool blank = true;
    for (char one : text) {
        bool const is_blank = one == ' ' || one == '\n' || one == '\t' || one == '\r';
        if (is_blank) {
            if (!blank) out += "  \n\t";
            blank = true;
        } else {
            out.push_back(one);
            blank = false;
        }
    }
    return "\n" + out + "\r\n";
}

}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    FuzzedDataProvider fdp(data, size);
    int const mode = fdp.ConsumeIntegralInRange<int>(0, 2);
    double const epsilon = fdp.PickValueInArray({0.0, 1e-9, 1e-6, 0.5});
    bool const same = fdp.ConsumeBool();
    std::string const answer = fdp.ConsumeRandomLengthString(512);
    std::string const output = same ? answer : fdp.ConsumeRemainingBytesAsString();
    eof::verdict const got = judge(mode, output, answer, epsilon);
    eof::must(got.stopped, "the checker ended without a verdict");
    eof::must(got.code == 0 || got.code == 1 || got.code == 3 || got.code == 7, "exit code");
    if (same) eof::must(got.code == 0, "identical files are not accepted");
    if (same && mode != 1) {
        eof::verdict const spaced = judge(mode, respaced(answer), answer, epsilon);
        eof::must(spaced.code == 0, "whitespace changes are not accepted by a token comparison");
    }
    return 0;
}
