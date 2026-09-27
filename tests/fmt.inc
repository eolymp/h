EO_TEST(fmt_without_values_copies_the_pattern) {
    EO_CHECK_EQ(eo::fmt("plain text"), std::string("plain text"));
    EO_CHECK_EQ(eo::fmt(""), std::string(""));
}

EO_TEST(fmt_substitutes_in_order) {
    EO_CHECK_EQ(eo::fmt("{} and {}", 1, 2), std::string("1 and 2"));
    EO_CHECK_EQ(eo::fmt("a[{}]", 5), std::string("a[5]"));
}

EO_TEST(fmt_escapes_braces) {
    EO_CHECK_EQ(eo::fmt("{{}}"), std::string("{}"));
    EO_CHECK_EQ(eo::fmt("{{{}}}", 7), std::string("{7}"));
}

EO_TEST(fmt_writes_integers_of_every_width) {
    EO_CHECK_EQ(eo::fmt("{}", 0), std::string("0"));
    EO_CHECK_EQ(eo::fmt("{}", -12), std::string("-12"));
    EO_CHECK_EQ(eo::fmt("{}", static_cast<short>(-3)), std::string("-3"));
    EO_CHECK_EQ(eo::fmt("{}", 9223372036854775807LL), std::string("9223372036854775807"));
    EO_CHECK_EQ(eo::fmt("{}", -9223372036854775807LL - 1), std::string("-9223372036854775808"));
    EO_CHECK_EQ(eo::fmt("{}", 18446744073709551615ULL), std::string("18446744073709551615"));
    EO_CHECK_EQ(eo::fmt("{}", static_cast<unsigned>(7)), std::string("7"));
    EO_CHECK_EQ(eo::fmt("{}", static_cast<std::size_t>(42)), std::string("42"));
}

EO_TEST(fmt_writes_reals_that_read_back_exactly) {
    EO_CHECK_EQ(eo::fmt("{}", 0.5), std::string("0.5"));
    EO_CHECK_EQ(eo::fmt("{}", 1.0f), std::string("1"));
    EO_CHECK_EQ(eo::fmt("{}", -0.25), std::string("-0.25"));
    double const awkward = 1.0 + 2.220446049250313e-16;
    std::string const written = eo::fmt("{}", awkward);
    EO_CHECK(std::strtod(written.c_str(), nullptr) == awkward);
    EO_CHECK(written.size() > 1);
}

EO_TEST(fmt_writes_a_fixed_number_of_any_length) {
    EO_CHECK_EQ(eo::fmt("[{}]", eo::fixed(2.5, 3)), std::string("[2.500]"));
    std::string const huge = eo::fmt("[{}]", eo::fixed(1e300, 2));
    EO_CHECK_EQ(huge.size(), static_cast<std::size_t>(306));
    EO_CHECK_EQ(huge.substr(0, 3), std::string("[10"));
    EO_CHECK_EQ(huge.substr(huge.size() - 4), std::string(".00]"));
    std::string const digits = eo::fmt("{}", eo::fixed(0.5, 100));
    EO_CHECK_EQ(digits, "0.5" + std::string(99, '0'));
    EO_CHECK_EQ(eo::fmt("{}", eo::fixed(0.5, 61)), "0.5" + std::string(60, '0'));
    EO_CHECK_EQ(eo::fmt("{}", eo::fixed(0.5, 62)), "0.5" + std::string(61, '0'));
}

EO_TEST(fmt_writes_text_of_every_kind) {
    char mutable_text[] = "mutable";
    char* pointer = mutable_text;
    char const* constant = "constant";
    char const* missing = nullptr;
    EO_CHECK_EQ(eo::fmt("{}", "literal"), std::string("literal"));
    EO_CHECK_EQ(eo::fmt("{}", constant), std::string("constant"));
    EO_CHECK_EQ(eo::fmt("{}", pointer), std::string("mutable"));
    EO_CHECK_EQ(eo::fmt("{}", missing), std::string("(null)"));
    EO_CHECK_EQ(eo::fmt("{}", std::string("owned")), std::string("owned"));
    EO_CHECK_EQ(eo::fmt("{}", std::string_view("viewed")), std::string("viewed"));
    EO_CHECK_EQ(eo::fmt("{}", 'c'), std::string("c"));
    EO_CHECK_EQ(eo::fmt("{} {}", true, false), std::string("true false"));
}

EO_TEST(fmt_refuses_a_lone_brace) {
    eot::outcome const opening = eot::run_until_stop([] { (void)eo::fmt("a { b"); });
    EO_CHECK(opening.stopped);
    EO_CHECK_IN(opening.text, "lone '{'");
    eot::outcome const closing = eot::run_until_stop([] { (void)eo::fmt("a } b"); });
    EO_CHECK(closing.stopped);
    EO_CHECK_IN(closing.text, "lone '}'");
}

EO_TEST(fmt_refuses_a_count_that_does_not_match) {
    eot::outcome const hungry = eot::run_until_stop([] { (void)eo::fmt("{} {}", 1); });
    EO_CHECK(hungry.stopped);
    EO_CHECK_IN(hungry.text, "more {} than the 1 values given");
    eot::outcome const spare = eot::run_until_stop([] { (void)eo::fmt("{}", 1, 2); });
    EO_CHECK(spare.stopped);
    EO_CHECK_IN(spare.text, "has 1 {} but 2 values were given");
}
