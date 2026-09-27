#pragma once

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "core.h"
#include "fmt.h"

namespace eo {
namespace detail {

inline int constexpr last_words_patience_ms = 500;
inline int constexpr last_words_deadline_ms = 2000;

inline void write_while_read(int descriptor, std::string const& bytes, int patience_ms, int deadline_ms) {
    int const flags = ::fcntl(descriptor, F_GETFL);
    if (flags >= 0) ::fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
    auto const started = std::chrono::steady_clock::now();
    std::size_t sent = 0;
    while (sent < bytes.size()) {
        ssize_t const wrote = ::write(descriptor, bytes.data() + sent, bytes.size() - sent);
        if (wrote > 0) {
            sent += static_cast<std::size_t>(wrote);
            continue;
        }
        if (wrote < 0 && errno == EINTR) continue;
        if (wrote == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) return;
        long long const spent = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - started)
                                    .count();
        if (spent >= deadline_ms) return;
        pollfd room{descriptor, POLLOUT, 0};
        int const ready = ::poll(&room, 1, static_cast<int>(std::min<long long>(patience_ms, deadline_ms - spent)));
        if (ready == 0 || (ready < 0 && errno != EINTR)) return;
    }
}

inline void write_all(int descriptor, char const* bytes, std::size_t size, bool& broken) {
    while (size > 0) {
        ssize_t const written = ::write(descriptor, bytes, size);
        if (written < 0) {
            if (errno == EINTR) continue;
            broken = true;
            return;
        }
        bytes += written;
        size -= static_cast<std::size_t>(written);
    }
}

inline bool file_is_there(char const* path) {
    int const descriptor = ::open(path, O_RDONLY);
    if (descriptor < 0) return false;
    ::close(descriptor);
    return true;
}

inline void write_file(std::string const& path, std::string const& bytes, char const* what) {
    std::FILE* const file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) library_error(fmt("cannot write the {} to {}", what, path));
    bool const whole = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    if (std::fclose(file) != 0 || !whole)
        library_error(fmt("the {} could not be written to {}: {}", what, path, std::strerror(errno)));
}

class source {
public:
    static std::size_t constexpr default_chunk = 1u << 20;

    source() = default;
    source(source const&) = delete;
    source& operator=(source const&) = delete;
    source(source&& other) noexcept { steal(other); }

    source& operator=(source&& other) noexcept {
        if (this != &other) {
            release();
            steal(other);
        }
        return *this;
    }

    ~source() { release(); }

    static source over_text(std::string_view text, bool normalize, std::size_t chunk = default_chunk) {
        source made(normalize, chunk);
        made.pending_ = text;
        made.text_backed_ = true;
        made.drained_ = text.empty();
        return made;
    }

    static source over_descriptor(int descriptor, bool owned, bool normalize,
                                  std::size_t chunk = default_chunk) {
        source made(normalize, chunk);
        made.descriptor_ = descriptor;
        made.owned_ = owned;
        made.drained_ = false;
        return made;
    }

    static source over_file(char const* path, bool normalize, std::size_t chunk = default_chunk) {
        int const descriptor = ::open(path, O_RDONLY);
        if (descriptor < 0) library_error(fmt("cannot open {}: {}", path, std::strerror(errno)));
        return over_descriptor(descriptor, true, normalize, chunk);
    }

    int peek() {
        for (;;) {
            if (!have(1)) return -1;
            int const here = static_cast<unsigned char>(buffer_[begin_]);
            if (!normalize_ || here != '\r') return here;
            if (!have(2)) return here;
            if (static_cast<unsigned char>(buffer_[begin_ + 1]) != '\n') return here;
            begin_++;
            carriage_returns_ = true;
        }
    }

    int take() {
        int const here = peek();
        if (here < 0) return here;
        begin_++;
        if (here == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        return here;
    }

    bool at_end() { return peek() < 0; }

    std::string ahead(std::size_t limit) {
        while (held() < limit && top_up()) {
        }
        return std::string(buffer_.data() + begin_, std::min(limit, end_ - begin_));
    }

    std::size_t held() const { return end_ - begin_; }

    bool top_up() {
        if (drained_) return false;
        if (!text_backed_) {
            int ready = 0;
            if (::ioctl(descriptor_, FIONREAD, &ready) != 0 || ready <= 0) return false;
        }
        return have(held() + 1);
    }

    long long line() const { return line_; }
    long long column() const { return column_; }
    bool carriage_returns() const { return carriage_returns_; }

private:
    source(bool normalize, std::size_t chunk)
        : buffer_(std::max<std::size_t>(chunk, 1)), normalize_(normalize) {}

    void release() {
        if (owned_ && descriptor_ >= 0) ::close(descriptor_);
        descriptor_ = -1;
        owned_ = false;
    }

    void steal(source& other) {
        buffer_ = std::move(other.buffer_);
        begin_ = other.begin_;
        end_ = other.end_;
        descriptor_ = other.descriptor_;
        owned_ = other.owned_;
        drained_ = other.drained_;
        normalize_ = other.normalize_;
        carriage_returns_ = other.carriage_returns_;
        pending_ = other.pending_;
        text_backed_ = other.text_backed_;
        line_ = other.line_;
        column_ = other.column_;
        other.descriptor_ = -1;
        other.owned_ = false;
        other.drained_ = true;
        other.begin_ = other.end_ = 0;
    }

    void compact() {
        if (begin_ == 0) return;
        std::memmove(buffer_.data(), buffer_.data() + begin_, end_ - begin_);
        end_ -= begin_;
        begin_ = 0;
    }

    bool have(std::size_t count) {
        while (end_ - begin_ < count && !drained_) {
            if (end_ == buffer_.size()) {
                compact();
                if (end_ == buffer_.size()) break;
            }
            std::size_t const room = buffer_.size() - end_;
            if (text_backed_) {
                std::size_t const taken = std::min(room, pending_.size());
                std::memcpy(buffer_.data() + end_, pending_.data(), taken);
                pending_.remove_prefix(taken);
                end_ += taken;
                if (pending_.empty()) drained_ = true;
                continue;
            }
            ssize_t const got = ::read(descriptor_, buffer_.data() + end_, room);
            if (got < 0) {
                if (errno == EINTR) continue;
                library_error(fmt("cannot read the input: {}", std::strerror(errno)));
            }
            if (got == 0) drained_ = true;
            end_ += static_cast<std::size_t>(got);
        }
        return end_ - begin_ >= count;
    }

    std::vector<char> buffer_;
    std::size_t begin_ = 0;
    std::size_t end_ = 0;
    int descriptor_ = -1;
    bool owned_ = false;
    bool drained_ = true;
    bool normalize_ = false;
    bool carriage_returns_ = false;
    std::string_view pending_;
    bool text_backed_ = false;
    long long line_ = 1;
    long long column_ = 1;
};

}  // namespace detail
}  // namespace eo
