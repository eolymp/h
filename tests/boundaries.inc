EO_TEST(a_sum_limit_allows_its_limit_and_refuses_one_more) {
    eot::outcome const exact = validate("2\n5 5\n", [](eo::validator& v) {
        int const t = v.read_int(1, 9, "t");
        v.read_eoln();
        eo::sum_limit total(10, "sum of n");
        for (int at = 0; at < t; at++) {
            total += v.read_int(1, 9, "n");
            if (at + 1 < t) v.read_space();
        }
        v.read_eoln();
    });
    EO_CHECK(!exact.stopped);

    eot::outcome const over = validate("2\n5 6\n", [](eo::validator& v) {
        int const t = v.read_int(1, 9, "t");
        v.read_eoln();
        eo::sum_limit total(10, "sum of n");
        for (int at = 0; at < t; at++) {
            total += v.read_int(1, 9, "n");
            if (at + 1 < t) v.read_space();
        }
        v.read_eoln();
    });
    EO_CHECK(over.stopped);
    EO_CHECK_IN(over.text, "sum of n is 11, above 10");
}

EO_TEST(a_sum_limit_refuses_a_total_that_overflows) {
    eot::outcome const wrapped = validate("2\n5000000000000000000 5000000000000000000\n", [](eo::validator& v) {
        int const t = v.read_int(1, 9, "t");
        v.read_eoln();
        eo::sum_limit total(8000000000000000000, "sum of a");
        for (int at = 0; at < t; at++) {
            total += v.read_long(1, 5000000000000000000, "a");
            if (at + 1 < t) v.read_space();
        }
        v.read_eoln();
    });
    EO_CHECK(wrapped.stopped);
    EO_CHECK_EQ(wrapped.code, 3);
    EO_CHECK_EQ(wrapped.text, std::string("sum of a does not fit a long long: 5000000000000000000 was added to "
                                          "5000000000000000000"));
    eot::outcome const below = validate("", [](eo::validator&) {
        eo::sum_limit total(0, "sum of a");
        total += std::numeric_limits<long long>::min();
        total += -1;
    });
    EO_CHECK_EQ(below.code, 3);
    EO_CHECK_IN(below.text, "sum of a does not fit a long long: -1 was added to -9223372036854775808");
}

EO_TEST(a_token_is_accepted_at_both_bounds_and_refused_outside_them) {
    for (auto const& one : std::vector<std::pair<std::string, bool>>{
             {"ab", false}, {"abc", true}, {"abcde", true}, {"abcdef", false}}) {
        eot::outcome const result = validate(one.first + "\n", [](eo::validator& v) {
            v.read_token(3, 5, eo::charset("a-z"), "s");
            v.read_eoln();
        });
        EO_CHECK_EQ(!result.stopped, one.second);
    }
}

EO_TEST(a_line_is_accepted_at_both_bounds_and_refused_outside_them) {
    for (auto const& one : std::vector<std::pair<std::string, bool>>{
             {"ab", false}, {"abc", true}, {"abcde", true}, {"abcdef", false}}) {
        eot::outcome const result = validate(one.first + "\n", [](eo::validator& v) {
            v.read_line(3, 5, eo::charset("a-z"), "title");
        });
        EO_CHECK_EQ(!result.stopped, one.second);
    }
}

EO_TEST(a_line_that_ends_in_crlf_measures_the_same_as_one_that_does_not) {
    for (auto const& one : std::vector<std::pair<std::string, bool>>{
             {"ab", false}, {"abc", true}, {"abcde", true}, {"abcdef", false}}) {
        eot::outcome const plain = check_run("1\n", one.first + "\n", "x\n", [](eo::checker& c) {
            c.input.read_int(1, 9, "n");
            c.output.read_line(3, 5, eo::charset("a-z"), "s");
            c.jury.skip_rest("nothing to compare");
            eo::accept();
        });
        eot::outcome const windows = check_run("1\n", one.first + "\r\n", "x\n", [](eo::checker& c) {
            c.input.read_int(1, 9, "n");
            c.output.read_line(3, 5, eo::charset("a-z"), "s");
            c.jury.skip_rest("nothing to compare");
            eo::accept();
        });
        EO_CHECK_EQ(plain.code == 0, one.second);
        EO_CHECK_EQ(windows.code, plain.code);
    }
}

EO_TEST(a_real_is_accepted_at_both_digit_bounds_and_refused_outside_them) {
    for (auto const& one : std::vector<std::pair<std::string, bool>>{
             {"0.5", false}, {"0.50", true}, {"0.5000", true}, {"0.50000", false}}) {
        eot::outcome const result = validate(one.first + "\n", [](eo::validator& v) {
            v.read_real(0.0, 1.0, 2, 4, "p");
            v.read_eoln();
        });
        EO_CHECK_EQ(!result.stopped, one.second);
    }
}

EO_TEST(a_self_loop_and_a_repeated_edge_are_not_a_simple_graph) {
    EO_CHECK(!static_cast<bool>(eo::is_simple_graph(3, {{1, 2}, {2, 2}})));
    EO_CHECK_IN(eo::is_simple_graph(3, {{1, 2}, {2, 2}}).message(), "loop at vertex 2");
    EO_CHECK(!static_cast<bool>(eo::is_simple_graph(3, {{1, 2}, {2, 1}})));
    EO_CHECK(static_cast<bool>(eo::is_simple_graph(3, {{1, 2}, {2, 3}})));
    EO_CHECK(!static_cast<bool>(eo::is_tree(3, {{1, 2}, {2, 2}})));
    EO_CHECK(!static_cast<bool>(eo::is_connected(3, {{1, 2}})));
}

EO_TEST(close_enough_holds_at_the_tolerance_and_not_past_it) {
    EO_CHECK(eo::close_enough(1.0, 1.0 + 1e-6, 1e-6));
    EO_CHECK(!eo::close_enough(1.0, 1.0 + 2e-6, 1e-6));
    EO_CHECK(eo::close_enough(1000000.0, 1000001.0, 1e-6));
    EO_CHECK(!eo::close_enough(1000000.0, 1000003.0, 1e-6));
    EO_CHECK(eo::close_enough(0.0, 0.0, 0.0));
    EO_CHECK(!eo::close_enough(0.0, 1e-9, 0.0));
}

EO_TEST(rounding_a_score_rounds_and_does_not_floor) {
    EO_CHECK_EQ(eo::detail::rounded(0.5, 0), 1.0);
    EO_CHECK_EQ(eo::detail::rounded(1.5, 0), 2.0);
    EO_CHECK_EQ(eo::detail::rounded(0.4, 0), 0.0);
    EO_CHECK_EQ(eo::detail::rounded(0.125, 2), 0.13);
    EO_CHECK_EQ(eo::detail::rounded(0.124, 2), 0.12);
}

EO_TEST(comparing_lines_trims_both_sides) {
    eot::outcome const jury_padded = check_run("1\n", "hello\n", "hello   \n", [](eo::checker& c) {
        c.input.read_int(1, 9, "n");
        c.lines();
    });
    EO_CHECK_EQ(jury_padded.code, 0);

    eot::outcome const output_padded = check_run("1\n", "hello   \n", "hello\n", [](eo::checker& c) {
        c.input.read_int(1, 9, "n");
        c.lines();
    });
    EO_CHECK_EQ(output_padded.code, 0);

    eot::outcome const different = check_run("1\n", "hello x\n", "hello\n", [](eo::checker& c) {
        c.input.read_int(1, 9, "n");
        c.lines();
    });
    EO_CHECK_EQ(different.code, 1);
}

EO_TEST(a_named_random_stream_is_not_the_default_one) {
    made const result = generate({"-n=1"}, [](eo::generator& g) {
        int const n = g.option<int>("n", 1, 10);
        long long const plain = g.rng().uniform(1, 1000000000);
        long long const named = g.rng("weights").uniform(1, 1000000000);
        long long const other = g.rng("labels").uniform(1, 1000000000);
        long long const again = g.rng("weights").uniform(1, 1000000000);
        g.out.line(n);
        g.out.line(plain != named ? 1 : 0, named != other ? 1 : 0, named != again ? 1 : 0);
    });
    EO_CHECK_IN(result.test, "1 1 1");
}

EO_TEST(two_call_sites_are_told_apart_by_file_as_well_as_line) {
    EO_CHECK(eo::detail::same_text("stream.h", "stream.h"));
    char const* const copy = "stream.h";
    std::string const built = std::string("stream") + ".h";
    EO_CHECK(eo::detail::same_text(copy, built.c_str()));
    EO_CHECK(!eo::detail::same_text("stream.h", "check.h"));
    EO_CHECK(!eo::detail::same_text(nullptr, "check.h"));
    EO_CHECK(!eo::detail::same_text("check.h", nullptr));
    EO_CHECK(eo::detail::same_text(nullptr, nullptr));
}

EO_TEST(a_rejection_loop_gives_up_instead_of_spinning) {
    long long tries = 0;
    for (int at = 0; at < 3; at++) eo::shapes::detail::still_trying(tries, 3, "a shape");
    eot::outcome const result = eot::run_until_stop([&] {
        long long spent = 0;
        for (int at = 0; at < 10; at++) eo::shapes::detail::still_trying(spent, 3, "a shape");
    });
    EO_CHECK(result.stopped);
    EO_CHECK_IN(result.text, "a shape did not come out in 3 attempts");
}

EO_TEST(a_capped_token_read_stops_where_it_was_told_to) {
    eo::detail::reader reading(eo::detail::source::over_text("abcdefghij rest\n", false),
                               eo::detail::fault::wrong_answer, "output.txt", true, "EO103");
    eo::detail::value_name const named("s");
    EO_CHECK_EQ(reading.take_word(named, eo::detail::site{"t", 1}, "a token", 4), std::string("abcd"));
    EO_CHECK_EQ(reading.take_word(named, eo::detail::site{"t", 2}, "a token", 0), std::string("efghij"));
    EO_CHECK(reading.longest_of({"?", "!"}) >= 64);
    EO_CHECK_EQ(reading.longest_of({"a-very-long-choice-that-is-more-than-sixty-four-characters-wide-indeed"}),
                71LL);
}
