EO_TEST(version_is_the_macro) {
    EO_CHECK_EQ(std::string(eo::version()), std::string(EOLYMP_H_VERSION));
}

EO_TEST(version_parts_are_the_numbers_in_the_version) {
    static_assert(eo::detail::version_part("12.34.56-rc1", 0) == 12);
    static_assert(eo::detail::version_part("12.34.56-rc1", 1) == 34);
    static_assert(eo::detail::version_part("12.34.56-rc1", 2) == 56);
    static_assert(eo::detail::version_part("1.0.10", 2) == 10);
    std::string const joined = std::to_string(EOLYMP_H_VERSION_MAJOR) + "." + std::to_string(EOLYMP_H_VERSION_MINOR) +
                               "." + std::to_string(EOLYMP_H_VERSION_PATCH);
    EO_CHECK_EQ(std::string(EOLYMP_H_VERSION).substr(0, joined.size()), joined);
#if EOLYMP_H_VERSION_MAJOR < 1
    EO_CHECK(false);
#endif
}

EO_TEST(environment_reads_the_process_environment) {
    ::unsetenv("EOLYMP_TEST_KNOB");
    EO_CHECK(eo::detail::environment("EOLYMP_TEST_KNOB") == nullptr);
    EO_CHECK(!eo::detail::environment_is("EOLYMP_TEST_KNOB", "1"));
    ::setenv("EOLYMP_TEST_KNOB", "2", 1);
    EO_CHECK(!eo::detail::environment_is("EOLYMP_TEST_KNOB", "1"));
    ::setenv("EOLYMP_TEST_KNOB", "1", 1);
    EO_CHECK(eo::detail::environment_is("EOLYMP_TEST_KNOB", "1"));
    ::unsetenv("EOLYMP_TEST_KNOB");
}

EO_TEST(judge_and_strict_come_from_the_environment) {
    ::unsetenv("EOLYMP");
    ::unsetenv("EOLYMP_STRICT");
    EO_CHECK(!eo::detail::on_judge());
    EO_CHECK(!eo::detail::strict_mode());
    ::setenv("EOLYMP", "1", 1);
    ::setenv("EOLYMP_STRICT", "1", 1);
    EO_CHECK(eo::detail::on_judge());
    EO_CHECK(eo::detail::strict_mode());
    ::unsetenv("EOLYMP");
    ::unsetenv("EOLYMP_STRICT");
}

EO_TEST(report_ends_every_message_with_one_line_break) {
    eot::redirect capture;
    eo::detail::report("");
    eo::detail::report("no break");
    eo::detail::report("has break\n");
    EO_CHECK_EQ(capture.finish(), std::string("no break\nhas break\n"));
}

EO_TEST(finish_carries_the_code_and_prints_the_text) {
    eot::outcome const result = eot::run_until_stop([] { eo::detail::finish(3, "invalid test"); });
    EO_CHECK(result.stopped);
    EO_CHECK_EQ(result.code, 3);
    EO_CHECK_EQ(result.text, std::string("invalid test"));
    EO_CHECK_EQ(result.printed, std::string("invalid test\n"));
}

EO_TEST(library_error_names_the_library) {
    eot::outcome const result = eot::run_until_stop([] { eo::detail::library_error("two validators"); });
    EO_CHECK(result.stopped);
    EO_CHECK_EQ(result.code, 3);
    EO_CHECK_EQ(result.text, std::string("eolymp.h: two validators"));
}

EO_TEST(any_and_unnamed_are_distinct_markers) {
    EO_CHECK(!std::is_same_v<eo::any_t, eo::unnamed_t>);
    EO_CHECK(std::is_same_v<decltype(eo::any), eo::any_t const>);
    EO_CHECK(std::is_same_v<decltype(eo::unnamed), eo::unnamed_t const>);
}
