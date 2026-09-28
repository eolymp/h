#include "fuzz.h"

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
    std::string const carried(reinterpret_cast<char const*>(data), size);
    eof::memfile in(carried), out("");
    char name[] = "interactor";
    char* argv[] = {name, in.path(), out.path(), nullptr};
    eof::verdict const got = eof::run([&] {
        eo::interactor it(3, argv);
        eo::detail::emitter() = &eof::quiet;
        eo::phases p(it, 3);
        (void)p.previous().at_eof();
        it.fail_jury("reached the body");
    });
    eo::detail::restore_channels reset;
    eof::must(got.stopped, "no verdict");
    return 0;
}
