#pragma once

#include <cmath>
#include <map>
#include <string>

#include "core.h"
#include "fmt.h"

namespace eo {

class summary {
public:
    double fraction() const { return fraction_; }
    std::string const& message() const { return message_; }

    double value(std::string const& name) const {
        auto const found = values_.find(name);
        if (found == values_.end())
            detail::library_error(fmt("the interactor recorded no value called \"{}\"", name));
        return found->second;
    }

    bool has(std::string const& name) const { return values_.count(name) != 0; }

    void set_fraction(double what) { fraction_ = what == 0 ? 0.0 : what; }
    void set_message(std::string what) {
        message_ = one_line(std::move(what));
    }

    static std::string one_line(std::string what) {
        for (char& one : what)
            if (one == '\n' || one == '\r') one = ' ';
        return what;
    }
    void record(std::string name, double what) {
        if (!std::isfinite(what))
            detail::library_error(fmt("the value \"{}\" is {}, which a summary cannot carry; record a finite number",
                                      name, what));
        values_[std::move(name)] = what == 0 ? 0.0 : what;
    }

    std::string written() const {
        std::string out = fmt("{} 1\n", marker());
        out += fmt("fraction {}\n", fraction_);
        for (auto const& one : values_) out += fmt("value {} {}\n", one.first, one.second);
        out += fmt("message {}\n", message_);
        return out;
    }

    static char const* marker() { return "eolymp-summary"; }

private:
    double fraction_ = 0;
    std::string message_;
    std::map<std::string, double> values_;
};

}  // namespace eo
