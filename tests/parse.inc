namespace {

eo::detail::number_problem integer_problem(char const* text) {
    return eo::detail::parse_integer(text).problem;
}

eo::detail::number_problem real_problem(char const* text, bool exponent = false) {
    return eo::detail::parse_real(text, exponent).problem;
}

}  // namespace

EO_TEST(describe_has_a_phrase_for_every_problem) {
    using eo::detail::describe;
    using eo::detail::number_problem;
    EO_CHECK_EQ(std::string(describe(number_problem::none)), std::string(""));
    EO_CHECK_IN(std::string(describe(number_problem::empty)), "empty");
    EO_CHECK_IN(std::string(describe(number_problem::missing_digits)), "digits");
    EO_CHECK_IN(std::string(describe(number_problem::bad_character)), "character");
    EO_CHECK_IN(std::string(describe(number_problem::leading_zero)), "leading zero");
    EO_CHECK_IN(std::string(describe(number_problem::redundant_minus)), "minus");
    EO_CHECK_IN(std::string(describe(number_problem::out_of_range)), "fit");
}

EO_TEST(integers_that_are_accepted) {
    EO_CHECK_EQ(eo::detail::parse_integer("0").value, 0LL);
    EO_CHECK_EQ(eo::detail::parse_integer("7").value, 7LL);
    EO_CHECK_EQ(eo::detail::parse_integer("-7").value, -7LL);
    EO_CHECK_EQ(eo::detail::parse_integer("1000000007").value, 1000000007LL);
    EO_CHECK_EQ(eo::detail::parse_integer("9223372036854775807").value, 9223372036854775807LL);
    EO_CHECK_EQ(eo::detail::parse_integer("-9223372036854775808").value, -9223372036854775807LL - 1);
    EO_CHECK(integer_problem("0") == eo::detail::number_problem::none);
}

EO_TEST(integers_follow_the_strict_syntax) {
    using eo::detail::number_problem;
    EO_CHECK(integer_problem("") == number_problem::empty);
    EO_CHECK(integer_problem("-") == number_problem::missing_digits);
    EO_CHECK(integer_problem("+5") == number_problem::bad_character);
    EO_CHECK(integer_problem("12a") == number_problem::bad_character);
    EO_CHECK(integer_problem("-a") == number_problem::bad_character);
    EO_CHECK(integer_problem(" 5") == number_problem::bad_character);
    EO_CHECK(integer_problem("007") == number_problem::leading_zero);
    EO_CHECK(integer_problem("00") == number_problem::leading_zero);
    EO_CHECK(integer_problem("-00") == number_problem::leading_zero);
    EO_CHECK(integer_problem("-0") == number_problem::redundant_minus);
    EO_CHECK(integer_problem("9223372036854775808") == number_problem::out_of_range);
    EO_CHECK(integer_problem("-9223372036854775809") == number_problem::out_of_range);
    EO_CHECK(integer_problem("99999999999999999999999") == number_problem::out_of_range);
}

EO_TEST(reals_that_are_accepted) {
    eo::detail::real_read const half = eo::detail::parse_real("0.5", false);
    EO_CHECK(half.problem == eo::detail::number_problem::none);
    EO_CHECK(half.value == 0.5);
    EO_CHECK_EQ(half.decimals, 1);
    EO_CHECK_EQ(eo::detail::parse_real("3", false).decimals, 0);
    EO_CHECK_EQ(eo::detail::parse_real("0.000", false).decimals, 3);
    EO_CHECK(eo::detail::parse_real("0.000", false).value == 0.0);
    EO_CHECK(eo::detail::parse_real("-12.25", false).value == -12.25);
    EO_CHECK_EQ(eo::detail::parse_real("-12.25", false).decimals, 2);
}

EO_TEST(reals_follow_the_strict_syntax) {
    using eo::detail::number_problem;
    EO_CHECK(real_problem("") == number_problem::empty);
    EO_CHECK(real_problem("-") == number_problem::missing_digits);
    EO_CHECK(real_problem(".5") == number_problem::missing_digits);
    EO_CHECK(real_problem("-.5") == number_problem::missing_digits);
    EO_CHECK(real_problem("5.") == number_problem::missing_digits);
    EO_CHECK(real_problem("00.5") == number_problem::leading_zero);
    EO_CHECK(real_problem("-0") == number_problem::redundant_minus);
    EO_CHECK(real_problem("-0.000") == number_problem::redundant_minus);
    EO_CHECK(real_problem("12a") == number_problem::bad_character);
    EO_CHECK(real_problem("1.2.3") == number_problem::bad_character);
    EO_CHECK(real_problem("+1.5") == number_problem::missing_digits);
}

EO_TEST(a_lenient_read_takes_negative_zero_as_zero) {
    using eo::detail::number_problem;
    eo::detail::real_read const printed = eo::detail::parse_real("-0.000000", false, true);
    EO_CHECK(printed.problem == number_problem::none);
    EO_CHECK(printed.value == 0 && !std::signbit(printed.value));
    EO_CHECK_EQ(printed.decimals, 6);
    eo::detail::real_read const tiny = eo::detail::parse_real("-1e-400", true, true);
    EO_CHECK(tiny.problem == number_problem::none);
    EO_CHECK(tiny.value == 0 && !std::signbit(tiny.value));
    EO_CHECK(eo::detail::parse_real("-1e-400", true).problem == number_problem::redundant_minus);
    EO_CHECK(eo::detail::parse_real("-0.5", false, true).value == -0.5);
}

EO_TEST(exponents_are_a_choice) {
    using eo::detail::number_problem;
    EO_CHECK(real_problem("1e5") == number_problem::bad_character);
    EO_CHECK(real_problem("1E5", true) == number_problem::none);
    EO_CHECK(eo::detail::parse_real("1e5", true).value == 100000.0);
    EO_CHECK(eo::detail::parse_real("1.5e-3", true).value == 0.0015);
    EO_CHECK(eo::detail::parse_real("1e+3", true).value == 1000.0);
    EO_CHECK(real_problem("1e", true) == number_problem::missing_digits);
    EO_CHECK(real_problem("1e+", true) == number_problem::missing_digits);
    EO_CHECK(real_problem("1e5x", true) == number_problem::bad_character);
    EO_CHECK(real_problem("1e400", true) == number_problem::out_of_range);
    EO_CHECK(real_problem("-1e400", true) == number_problem::out_of_range);
}
