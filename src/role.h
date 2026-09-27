#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include "core.h"
#include "diag.h"
#include "fmt.h"
#include "stream.h"

namespace eo {

struct rounding {
    int digits;
};

inline rounding round_to(int digits) { return rounding{digits}; }

inline double ratio(long long part, long long whole) {
    if (whole == 0) detail::library_error("eo::ratio divides by zero");
    return static_cast<double>(part) / static_cast<double>(whole);
}

inline bool close_enough(double expected, double found, double epsilon) {
    double const spread = std::fabs(expected - found);
    if (spread <= epsilon) return true;
    double const scale = std::fabs(expected);
    return scale > 0 && spread / scale <= epsilon;
}

namespace detail {

class scorer {
public:
    virtual double cost() const = 0;
    virtual void pass(double fraction, std::string const& message) = 0;
    virtual void fail_run(std::string const& message) = 0;
    virtual void fail_jury(std::string const& message) = 0;

protected:
    ~scorer() = default;
};

class limits_keeper {
public:
    virtual void declare_budget() = 0;
    virtual void spent_a_budget() = 0;

protected:
    ~limits_keeper() = default;
};

inline scorer*& live_scorer() {
    static scorer* only = nullptr;
    return only;
}

inline scorer& judging() {
    if (live_scorer() == nullptr) library_error("this verdict needs an eo::checker or an eo::interactor");
    return *live_scorer();
}

inline reader*& blaming() {
    static reader* current = nullptr;
    return current;
}

class blame_guard {
public:
    explicit blame_guard(reader* who) : before_(blaming()) { blaming() = who; }
    blame_guard(blame_guard const&) = delete;
    blame_guard& operator=(blame_guard const&) = delete;
    ~blame_guard() { blaming() = before_; }

private:
    reader* before_;
};

[[noreturn]] inline void refuse_a_score(std::string const& what) {
    library_error(fmt("{} is not a number the judge can pay; look for zero divided by zero, or an infinity "
                      "less an infinity, in the formula", what));
}

inline double clamped(double fraction) {
    if (std::isnan(fraction)) refuse_a_score(fmt("a score of {}", fraction));
    if (fraction >= 2 && std::isfinite(fraction)) {
        warn("EO205", fmt("a score of {} was clamped to 1; it looks like a percentage or points, and eo::score "
                          "takes a fraction of the test", fraction),
             "use eo::ratio(a, b) for a out of b, or eo::points for points", site::here());
        return 1.0;
    }
    if (fraction < 0 || fraction > 1) {
        warn("EO205", fmt("a score of {} was clamped into 0..1", fraction), "keep the formula inside the test",
             site::here());
        return fraction < 0 ? 0.0 : 1.0;
    }
    if (fraction > 0 && fraction < 1 && fraction > 1 - 1e-9)
        warn("EO206", fmt("a score of {} is a hair below full marks", fraction),
             "use eo::ratio(a, b), which is exact", site::here());
    return fraction;
}

inline std::string format_points(double value) {
    if (value == std::floor(value) && std::fabs(value) < 1e15) {
        std::string out;
        append_integer(out, static_cast<long long>(value));
        return out;
    }
    char buffer[40];
    int const written = std::snprintf(buffer, sizeof(buffer), "%.10g", value);
    return std::string(buffer, static_cast<std::size_t>(written));
}

inline double rounded(double value, int digits) {
    if (digits > 15) return value;
    double scale = 1;
    for (int at = 0; at < digits; at++) scale *= 10;
    return std::round(value * scale) / scale;
}

}  // namespace detail

class budget {
public:
    template <class Owner>
    budget(Owner& owner, long long limit, std::string name)
        : keeper_(&owner), judge_(&owner), limit_(limit), name_(std::move(name)) {
        keeper_->declare_budget();
    }

    budget(budget const&) = delete;
    budget& operator=(budget const&) = delete;

    void spend(long long how_many = 1) {
        keeper_->spent_a_budget();
        used_ += how_many;
        if (used_ > limit_) judge_->fail_run(fmt("more than {} {}", limit_, name_));
    }

    void restart() { used_ = 0; }

    void restart(long long limit) {
        limit_ = limit;
        used_ = 0;
    }

    long long used() const { return used_; }
    long long left() const { return limit_ - used_; }
    long long limit() const { return limit_; }

private:
    detail::limits_keeper* keeper_;
    detail::scorer* judge_;
    long long limit_;
    long long used_ = 0;
    std::string name_;
};

template <class... Args>
[[noreturn]] inline void accept(std::string_view pattern = "", Args const&... args) {
    detail::judging().pass(1, fmt(pattern, args...));
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
[[noreturn]] inline void wrong(std::string_view pattern = "", Args const&... args) {
    std::string const message = fmt(pattern, args...);
    if (detail::blaming() != nullptr) detail::blaming()->refuse(detail::value_name(unnamed), message);
    detail::judging().fail_run(message);
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
[[noreturn]] inline void jury_error(std::string_view pattern = "", Args const&... args) {
    detail::judging().fail_jury(fmt(pattern, args...));
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
[[noreturn]] inline void score(double fraction, std::string_view pattern = "", Args const&... args) {
    detail::judging().pass(detail::clamped(fraction), fmt(pattern, args...));
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
[[noreturn]] inline void score(double fraction, rounding how, std::string_view pattern = "",
                               Args const&... args) {
    detail::scorer& one = detail::judging();
    double const paid = detail::rounded(detail::clamped(fraction) * one.cost(), how.digits);
    one.pass(one.cost() > 0 ? paid / one.cost() : 0, fmt(pattern, args...));
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
[[noreturn]] inline void points(double paid, std::string_view pattern = "", Args const&... args) {
    detail::scorer& one = detail::judging();
    if (std::isnan(paid)) detail::refuse_a_score(fmt("{} points", paid));
    if (paid > one.cost())
        detail::warn("EO207", fmt("{} points is more than the test's {}", paid, one.cost()),
                     "the judge clamps it", detail::site::here());
    one.pass(one.cost() > 0 ? paid / one.cost() : 0, fmt(pattern, args...));
    __builtin_unreachable();  // LCOV_EXCL: the verdict above ends the program
}

template <class... Args>
inline void log(std::string_view pattern, Args const&... args) {
    detail::log_line(fmt(pattern, args...));
}

}  // namespace eo
