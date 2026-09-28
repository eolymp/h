#pragma once
#include "../../eolymp.h"

#include <cstdint>
#include <cstdlib>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

namespace eof {

inline void quiet(std::string const&) {}

inline void fresh() {
    eo::detail::diagnostics::shared().forget_everything();
    eo::detail::emitter() = &quiet;
    eo::detail::current_case() = 0;
}

struct verdict {
    bool stopped = false;
    int code = 0;
    std::string text;
    bool operator==(verdict const& o) const {
        return stopped == o.stopped && code == o.code && context_free(text) == context_free(o.text);
    }
    static std::string context_free(std::string const& text) {
        std::size_t const quoted = text.find(" \"");
        return quoted == std::string::npos ? text : text.substr(0, quoted);
    }
};

template <class Body>
verdict run(Body&& body) {
    fresh();
    verdict out;
    try {
        body();
    } catch (eo::detail::stop const& reached) {
        out.stopped = true;
        out.code = reached.code;
        out.text = reached.text;
    }
    eo::detail::emitter() = nullptr;
    return out;
}

class memfile {
public:
    explicit memfile(std::string const& bytes) {
        fd_ = ::memfd_create("eo-fuzz", 0);
        if (fd_ < 0) std::abort();
        std::size_t done = 0;
        while (done < bytes.size()) {
            ssize_t const wrote = ::write(fd_, bytes.data() + done, bytes.size() - done);
            if (wrote <= 0) std::abort();
            done += static_cast<std::size_t>(wrote);
        }
        path_ = "/proc/self/fd/" + std::to_string(fd_);
    }
    memfile(memfile const&) = delete;
    ~memfile() { ::close(fd_); }
    char* path() { return path_.data(); }

private:
    int fd_ = -1;
    std::string path_;
};

inline int loud() {
    static int const kept = ::dup(2);
    return kept;
}

inline bool const loud_is_kept = loud() >= 0;

inline void must(bool condition, char const* what) {
    if (!condition) {
        ::dprintf(loud(), "property violated: %s\n", what);
        std::abort();
    }
}

}
