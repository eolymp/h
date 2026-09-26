#pragma once

#include <array>
#include <string>
#include <utility>

#include "core.h"
#include "diag.h"
#include "fmt.h"

namespace eo {

class charset {
public:
    explicit charset(char const* spec) : spec_(spec) { build(); }
    explicit charset(std::string spec) : spec_(std::move(spec)) { build(); }

    bool has(char c) const { return allowed_[static_cast<unsigned char>(c)]; }
    std::string const& text() const { return spec_; }

private:
    void build() {
        allowed_.fill(false);
        for (std::size_t at = 0; at < spec_.size(); at++) {
            if (at + 2 < spec_.size() && spec_[at + 1] == '-') {
                unsigned const from = static_cast<unsigned char>(spec_[at]);
                unsigned const to = static_cast<unsigned char>(spec_[at + 2]);
                if (from > to)
                    detail::library_error(fmt("the character range \"{}\" in charset(\"{}\") runs backwards",
                                              spec_.substr(at, 3), spec_));
                for (unsigned c = from; c <= to; c++) allowed_[c] = true;
                at += 2;
            } else {
                allowed_[static_cast<unsigned char>(spec_[at])] = true;
            }
        }
    }

    std::array<bool, 256> allowed_{};
    std::string spec_;
};

namespace detail {

enum class stated { yes, deliberate, absent };

class value_name {
public:
    value_name(char const* text) : text_(text), state_(stated::yes) {}
    value_name(std::string text) : text_(std::move(text)), state_(stated::yes) {}
    value_name(unnamed_t) : state_(stated::deliberate) {}

    static value_name nothing() {
        value_name made{unnamed};
        made.state_ = stated::absent;
        return made;
    }

    bool known() const { return state_ == stated::yes; }
    bool absent() const { return state_ == stated::absent; }
    std::string const& text() const { return text_; }

    value_name at(long long index) const {
        if (!known()) return *this;
        value_name made(fmt("{}[{}]", text_, index));
        made.key_ = text_;
        return made;
    }

    std::string const& key() const { return key_.empty() ? text_ : key_; }

private:
    std::string text_;
    std::string key_;
    stated state_;
};

inline bool is_round(long long value) {
    if (value < 1000) return false;
    while (value % 10 == 0) value /= 10;
    return value == 1 || value == 2 || value == 5;
}

inline bool nearly_round(long long value) {
    return !is_round(value) && (is_round(value - 1) || is_round(value + 1));
}

inline std::string shorten(std::string const& text, std::size_t limit = 40) {
    if (text.size() <= limit) return text;
    return text.substr(0, limit) + "...";
}

inline char const* name_of(int character) {
    if (character < 0) return "the end of the input";
    if (character == ' ') return "a space";
    if (character == '\n') return "a line break";
    if (character == '\t') return "a tab";
    if (character == '\r') return "a carriage return";
    return "";
}

inline bool same_folded(std::string const& left, char const* right) {
    std::size_t at = 0;
    for (; at < left.size() && right[at] != '\0'; at++) {
        char const one = left[at] >= 'A' && left[at] <= 'Z' ? static_cast<char>(left[at] + 32) : left[at];
        char const other = right[at] >= 'A' && right[at] <= 'Z' ? static_cast<char>(right[at] + 32) : right[at];
        if (one != other) return false;
    }
    return at == left.size() && right[at] == '\0';
}

inline bool is_blank(int character) {
    return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

}  // namespace detail

inline detail::value_name element(std::string name, long long index) {
    return detail::value_name(std::move(name)).at(index);
}

}  // namespace eo
