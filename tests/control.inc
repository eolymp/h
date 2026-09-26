namespace {

struct staged {
    bool stopped = false;
    int code = 0;
    std::string text;
    std::string asked;
    std::string said;
    std::string log;
    std::string summary;
};

template <class Body>
staged run_control(std::string const& test, std::vector<std::string> const& replies,
                   std::vector<std::string> const& heard, long long limit, Body&& body,
                   bool with_answer = false) {
    eo::detail::diagnostics::shared().forget_everything();
    forget_the_environment();
    ::setenv("TEST_COST", "40", 1);
    ::setenv("INSTANCE_LIMIT", std::to_string(limit).c_str(), 1);
    scratch_file input(test);
    scratch_file answer(with_answer ? "7\n" : "");
    std::string const out_path = temporary_path();
    std::string const requests = temporary_path();
    std::string const complaints = temporary_path();
    std::vector<std::string> towards;
    std::string filled;
    for (std::size_t at = 0; at < replies.size(); at++) {
        if (replies[at] == "LIMIT" || replies[at] == "NONSENSE") {
            filled += replies[at] + "\n";
            continue;
        }
        towards.push_back(temporary_path());
        std::string const back = temporary_path();
        three_files::put(back, at < heard.size() ? heard[at] : std::string());
        towards.push_back(back);
        filled += towards[towards.size() - 2] + " " + back + "\n";
    }
    std::string const answers = temporary_path();
    three_files::put(answers, filled);
    ::setenv("CONTROL_OUTPUT_FILE", requests.c_str(), 1);
    ::setenv("CONTROL_INPUT_FILE", answers.c_str(), 1);
    staged result;
    {
        swapped_descriptors kept;
        int const grumbles = ::open(complaints.c_str(), O_WRONLY | O_TRUNC);
        ::dup2(grumbles, 2);
        std::string program = "controller.cpp";
        std::vector<char*> arguments{program.data(), const_cast<char*>(input.path.c_str()),
                                     const_cast<char*>(out_path.c_str())};
        if (with_answer) arguments.push_back(const_cast<char*>(answer.path.c_str()));
        try {
            eo::controller ctl(static_cast<int>(arguments.size()), arguments.data());
            body(ctl);
        } catch (eo::detail::stop const& reached) {
            result.stopped = true;
            result.code = reached.code;
            result.text = reached.text;
        }
        std::fflush(stderr);
        ::close(grumbles);
    }
    result.asked = slurp(requests);
    result.log = slurp(complaints);
    result.summary = slurp(out_path);
    for (std::size_t at = 0; at + 1 < towards.size(); at += 2) result.said += slurp(towards[at]);
    for (std::string const& one : towards) ::unlink(one.c_str());
    ::unlink(out_path.c_str());
    ::unlink(requests.c_str());
    ::unlink(answers.c_str());
    ::unlink(complaints.c_str());
    ::unsetenv("CONTROL_OUTPUT_FILE");
    ::unsetenv("CONTROL_INPUT_FILE");
    forget_the_environment();
    if (std::getenv("EOT_TRACE") != nullptr) std::fprintf(stderr, "   control: %s\n", result.text.c_str());
    return result;
}

}  // namespace

EO_TEST(a_controller_starts_instances_and_relays_between_them) {
    staged const result = run_control("3 12345\n", {"ok", "ok", "ok"}, {"abc\n", "abc\n", "12345\n"}, 10,
                                      [](eo::controller& ctl) {
        int const k = ctl.input.read_int(2, 100, "instances");
        long long const secret = ctl.input.read_long(eo::any, "secret");
        EO_CHECK_EQ(ctl.instance_limit(), 10LL);
        eo::channel& first = ctl.spawn();
        EO_CHECK_EQ(first.index(), 1LL);
        first.send("first", secret);
        std::string message = first.read_token(1, 20, eo::charset("a-z"), "message");
        first.close();
        for (int at = 2; at < k; at++) {
            eo::channel& middle = ctl.spawn();
            middle.send("middle", message);
            message = middle.read_token(1, 20, eo::charset("a-z"), "message");
            middle.close();
        }
        eo::channel& last = ctl.spawn();
        last.send("last", message);
        long long const said = last.read_long(eo::any, "secret");
        if (said != secret) eo::wrong("the last said {}", said);
        eo::accept("the secret crossed {} instances", k);
    });
    EO_CHECK_EQ(result.code, 0);
    EO_CHECK_EQ(result.text, std::string("ok the secret crossed 3 instances"));
    EO_CHECK_EQ(result.asked, std::string("SPAWN\nSPAWN\nSPAWN\n"));
    EO_CHECK_EQ(result.said, std::string("first 12345\nmiddle abc\nlast abc\n"));
    EO_CHECK_IN(result.summary, "fraction 1");
    EO_CHECK_IN(result.log, "3 instances, 3 round trips");
}

EO_TEST(an_instance_beyond_the_limit_is_the_problem_not_the_solution) {
    staged const result = run_control("3 1\n", {"LIMIT"}, {}, 2, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        one.send("hello");
        eo::accept();
    });
    EO_CHECK_EQ(result.code, 3);
    EO_CHECK_IN(result.text, "the judge refused instance 1; instance_limit is 2");
    EO_CHECK_IN(result.text, "control.inc:");
}

EO_TEST(a_controller_blames_the_instance_it_was_reading) {
    staged const nonsense = run_control("3 1\n", {"ok"}, {"maybe\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        one.send("go");
        one.read_int(1, 9, "x");
        eo::accept();
    });
    EO_CHECK_EQ(nonsense.code, 1);
    EO_CHECK_IN(nonsense.text, "wrong answer: instance 1, line 1, x: expected an integer, found \"maybe\"");
    staged const silent = run_control("3 1\n", {"ok"}, {""}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        one.send("go");
        one.read_int(1, 9, "x");
        eo::accept();
    });
    EO_CHECK_EQ(silent.code, 1);
    EO_CHECK_IN(silent.text, "instance 1 ended the dialogue early");
}

EO_TEST(every_channel_is_flushed_before_the_controller_waits_on_any) {
    staged const result = run_control("3 1\n", {"ok", "ok"}, {"5\n", "6\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        eo::channel& two = ctl.spawn();
        one.send("to one");
        two.send("to two");
        EO_CHECK_EQ(one.read_int(1, 9, "x"), 5);
        EO_CHECK_EQ(two.read_int(1, 9, "x"), 6);
        eo::accept("both answered");
    });
    EO_CHECK_EQ(result.code, 0);
    EO_CHECK_EQ(result.said, std::string("to one\nto two\n"));
    EO_CHECK_IN(result.log, "2 instances, 1 round trips");
}

EO_TEST(a_controller_can_award_part_of_the_test) {
    staged const result = run_control("3 1\n", {"ok"}, {"5\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        one.send("go");
        EO_CHECK_EQ(one.read_long(eo::any, "x"), 5LL);
        ctl.value("right", 2);
        eo::score(eo::ratio(2, 3), "2 of 3 instances named the maximum");
    });
    EO_CHECK_EQ(result.code, 0);
    EO_CHECK_IN(result.summary, "fraction 0.666");
    EO_CHECK_IN(result.summary, "value right 2");
}

EO_TEST(a_controller_reports_what_it_never_used) {
    staged const idle = run_control("3 1\n", {"ok", "ok"}, {"5\n", "5\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        (void)ctl.spawn();
        one.send("go");
        one.read_int(1, 9, "x");
        eo::accept();
    });
    EO_CHECK_EQ(idle.code, 0);
    EO_CHECK_IN(checker_warnings(), "warning EO408: instance 2 was started and never talked to");
    staged const deaf = run_control("3 1\n", {"ok"}, {"5\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        one.send("go");
        eo::accept("never listened");
    });
    EO_CHECK_EQ(deaf.code, 0);
    EO_CHECK_IN(checker_warnings(), "warning EO405: the controller accepted without reading anything");
    staged const unspent = run_control("3 1\n", {"ok"}, {"5\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::budget queries(ctl, 10, "queries");
        eo::channel& one = ctl.spawn();
        one.send("go");
        one.read_int(1, 9, "x");
        eo::accept();
    });
    EO_CHECK_EQ(unspent.code, 0);
    EO_CHECK_IN(checker_warnings(), "warning EO403: a budget was declared and never spent");
}

EO_TEST(a_budget_limits_one_instance) {
    staged const result = run_control("3 1\n", {"ok"}, {"1\n2\n3\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::channel& one = ctl.spawn();
        eo::budget queries(ctl, 2, "queries");
        for (;;) {
            one.send("?");
            one.read_int(1, 9, "x");
            queries.spend();
        }
    });
    EO_CHECK_EQ(result.code, 1);
    EO_CHECK_EQ(result.text, std::string("wrong answer more than 2 queries"));
}

EO_TEST(a_controller_needs_a_control_channel_and_only_one_role) {
    forget_the_environment();
    scratch_file input("3 1\n");
    std::string const out_path = temporary_path();
    std::string program = "controller.cpp";
    std::vector<char*> arguments{program.data(), const_cast<char*>(input.path.c_str()),
                                 const_cast<char*>(out_path.c_str())};
    eot::outcome const lonely = eot::run_until_stop([&] {
        eo::controller ctl(static_cast<int>(arguments.size()), arguments.data());
        (void)ctl.spawn();
        eo::accept();
    });
    EO_CHECK_IN(lonely.text, "there is no control channel");
    std::vector<char*> bare{program.data()};
    eot::outcome const naked = eot::run_until_stop([&] {
        eo::controller ctl(static_cast<int>(bare.size()), bare.data());
        (void)ctl.cost();
    });
    EO_CHECK_IN(naked.text, "a controller needs the test and a file for its summary");
    eot::outcome const twice = eot::run_until_stop([&] {
        eo::controller first(static_cast<int>(arguments.size()), arguments.data());
        eo::controller second(static_cast<int>(arguments.size()), arguments.data());
        (void)second.cost();
        eo::accept();
    });
    EO_CHECK_IN(twice.text, "this program already has a controller");
    ::unlink(out_path.c_str());
}

EO_TEST(a_controller_ends_the_test_itself) {
    staged const nothing = run_control("3 1\n", {}, {}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
    });
    EO_CHECK_EQ(nothing.code, 3);
    EO_CHECK_IN(nothing.text, "the controller ended without a verdict");
    staged const broken = run_control("3 1\n", {}, {}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        EO_CHECK(!ctl.has_jury());
        eo::jury_error("the test makes no sense");
    });
    EO_CHECK_EQ(broken.code, 3);
    staged const quiet = run_control("3 1\n", {}, {}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        eo::wrong();
    });
    EO_CHECK_EQ(quiet.code, 1);
    EO_CHECK_IN(checker_warnings(), "warning EO204: this wrong answer carries no message");
    staged const answered = run_control("3 1\n", {}, {}, 10, [](eo::controller& ctl) {
        EO_CHECK(ctl.has_jury());
        EO_CHECK_EQ(ctl.jury.read_int(1, 9, "a"), 7);
        ctl.input.read_int(2, 100, "instances");
        EO_CHECK(ctl.rng().uniform(1, 1000000) == ctl.rng().uniform(1, 1000000) ||
                 ctl.round_trips() == 0);
        eo::accept("nothing to do");
    }, true);
    EO_CHECK_EQ(answered.code, 0);
}

EO_TEST(a_dialogue_the_pipes_cannot_carry_is_reported) {
    std::string many;
    for (long long at = 0; at < 100002; at++) many += "1\n";
    staged const result = run_control("3 1\n", {"ok"}, {many}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        EO_CHECK(ctl.cost() == 40);
        eo::channel& one = ctl.spawn();
        for (long long at = 0; at < 100002; at++) {
            one.send(at);
            one.read_int(1, 9, "x");
        }
        eo::accept("{} round trips", ctl.round_trips());
    });
    EO_CHECK_EQ(result.code, 0);
    std::string const said = checker_warnings();
    EO_CHECK_IN(said, "note EO401: this run made 100002 round trips");
    EO_CHECK_IN(said, "warning EO402: 100002 round trips were answered with no eo::budget declared");
}

EO_TEST(a_judge_that_answers_nonsense_is_a_jury_error) {
    staged const odd = run_control("3 1\n", {"ok"}, {"5\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        (void)ctl.spawn();
        (void)ctl.spawn();
        eo::accept();
    });
    EO_CHECK_EQ(odd.code, 3);
    EO_CHECK_IN(odd.text, "the judge closed the control channel");
    staged const garbled = run_control("3 1\n", {"NONSENSE"}, {}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(2, 100, "instances");
        (void)ctl.spawn();
        eo::accept();
    });
    EO_CHECK_EQ(garbled.code, 3);
    EO_CHECK_IN(garbled.text, "the judge answered SPAWN with \"NONSENSE\"");
}


EO_TEST(a_last_word_to_an_instance_is_delivered_with_the_verdict) {
    staged const result = run_control("1\n", {"ok"}, {"7\n"}, 10, [](eo::controller& ctl) {
        ctl.input.read_int(1, 9, "n");
        eo::channel& only = ctl.spawn();
        only.send("guess");
        only.read_int(1, 9, "x");
        only.send("done");
        eo::accept("heard");
    });
    EO_CHECK_EQ(result.code, 0);
    EO_CHECK_EQ(result.said, std::string("guess\ndone\n"));
}
