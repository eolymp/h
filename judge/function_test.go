package main

import (
	"context"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestAFunctionProblemsTemplatesAreReadStrictly(t *testing.T) {
	t.Parallel()
	cpp20 := `{"runtime": "cpp:20-gnu14", "header": "h.cpp", "source": "s.cpp", "footer": "f.cpp"}`
	cpp23 := `{"runtime": "cpp:23-gnu14", "header": "h.cpp", "footer": "f.cpp"}`
	python := `{"runtime": "python:3.14-python", "header": "h.py", "footer": "f.py"}`
	for body, want := range map[string]string{
		`{"type": "FUNCTION"}`: `a FUNCTION problem needs templates, one per runtime: {"runtime": "cpp:20-gnu14", "header": …, "source": …, "footer": …}`,
		`{"type": "FUNCTION", "templates": [{"header": "h.cpp"}]}`:                                                                       `template 1 has no runtime`,
		`{"type": "FUNCTION", "templates": [` + cpp20 + `, ` + cpp20 + `]}`:                                                              `templates 1 and 2 are both for "cpp:20-gnu14"; the judge keeps one template per runtime`,
		`{"templates": [{"runtime": "cpp:20-gnu14", "header": "h.cpp"}]}`:                                                                `template 1 has a header or a footer, which the judge wraps around a submission on a FUNCTION problem only`,
		`{"type": "OUTPUT", "templates": [{"runtime": "cpp:20-gnu14", "source": "s.cpp"}]}`:                                              `an OUTPUT problem has no templates: its contestants upload files, not code`,
		`{"type": "FUNCTION", "templates": [` + cpp20 + `], "solutions": [{"name": "a", "source": "a.cpp", "runtime": "cpp:23-gnu14"}]}`: `solution "a" is written for "cpp:23-gnu14", and the problem has no template for it; the judge wraps a FUNCTION problem's solution in its runtime's template`,
		`{"type": "FUNCTION", "templates": [` + cpp20 + `, ` + cpp23 + `], "solutions": [{"name": "a", "source": "a.cpp"}]}`:             `solution "a" gives no runtime, and the problem has templates for 2 C++ runtimes, cpp:20-gnu14 and cpp:23-gnu14; give it a runtime`,
		`{"type": "FUNCTION", "templates": [` + python + `], "solutions": [{"name": "a", "source": "a.cpp"}]}`:                           `solution "a" gives no runtime, and the problem has no template for a C++ runtime; eo-judge builds C++ solutions only`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {}, "runtime": "cpp:20-gnu14"}]}`:                                     `solution "a" of an OUTPUT problem has a runtime; its outputs are files, not code`,
		`{"solutions": [{"name": "a", "source": "a.py", "runtime": "python:3.14-python"}]}`:                                              `solution "a" is written for "python:3.14-python", and eo-judge builds C++ solutions only`,
	} {
		err := loading(t, body)
		if err == nil || !strings.Contains(err.Error(), want) {
			t.Errorf("%s gave %v, not %q", body, err, want)
		}
	}
	problem := loaded(t, `{"type": "FUNCTION", "templates": [`+python+`, `+cpp20+`],
		"solutions": [{"name": "a", "source": "a.cpp"}, {"name": "b", "source": "b.cpp", "runtime": "cpp:20-gnu14"}]}`)
	for _, one := range problem.Solutions {
		if program := problem.programOf(one); program.wrapped == nil || program.wrapped.Runtime != "cpp:20-gnu14" ||
			program.Runtime != "cpp:20-gnu14" {
			t.Errorf("solution %s is built as %+v", one.Name, program)
		}
	}
	plain := loaded(t, `{"templates": [{"runtime": "cpp:20-gnu14", "source": "s.cpp"}],
		"solutions": [{"name": "a", "source": "a.cpp", "runtime": "cpp:23-gnu14"}]}`)
	if program := plain.programOf(plain.Solutions[0]); program.wrapped != nil || program.Runtime != "cpp:23-gnu14" {
		t.Errorf("a PROGRAM problem's solution is built as %+v", program)
	}
}

func TestRunWrapsAFunctionSolutionInItsTemplate(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	shop := workshop(t, "testdata/function")
	shop.Problem.Solutions = shop.Problem.Judged("")
	judged := judgeAll(t, shop)
	if got := judged["main"]; got.Verdict != Accepted || got.Score != 100 {
		t.Errorf("main scored %v at %v", got.Verdict, got.Score)
	}
	if got := judged["first-two"]; got.Verdict != WrongAnswer || got.Score != 60 {
		t.Errorf("first-two scored %v at %v", got.Verdict, got.Score)
	}
	var whole []byte
	for _, part := range []string{"templates/cpp-header.cpp", "main.cpp", "templates/cpp-footer.cpp"} {
		body, err := os.ReadFile(filepath.Join("testdata/function", part))
		if err != nil {
			t.Fatal(err)
		}
		whole = append(whole, body...)
	}
	built, err := os.ReadFile(filepath.Join(shop.Dir, solutionName("main"), "source.cpp"))
	if err != nil || string(built) != string(whole) {
		t.Errorf("main was built from %q (%v), not header, source and footer as they are:\n%q", built, err, whole)
	}
}

func TestAFunctionSolutionWithItsOwnMainNamesTheTemplate(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	shop := workshop(t, "testdata/function")
	_, err := shop.Evaluate(context.Background(), shop.Problem.Solution("with-main"))
	if err == nil || !strings.Contains(err.Error(), `solution.with-main does not compile inside the template for cpp:20-gnu14, `+
		`which is header, source and footer in one file; a FUNCTION problem's solution is the function alone, and `+
		`the template gives the rest, main() included`) || !strings.Contains(err.Error(), "solution.cpp:") {
		t.Errorf("a solution with its own main gave %v", err)
	}
}

func checkedTemplate(t *testing.T, change func(problem *Problem)) Findings {
	t.Helper()
	shop := workshop(t, "testdata/function")
	change(shop.Problem)
	found, err := shop.Check(context.Background(), false)
	if err != nil {
		t.Fatal(err)
	}
	return found
}

func TestCheckReadsAFunctionProblemsTemplates(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	for name, want := range map[string]struct {
		change func(problem *Problem)
		code   string
		said   string
	}{
		"a stub that solves the problem": {func(problem *Problem) {
			problem.Templates[0].Source = "templates/cpp-source-solved.cpp"
		}, "EO823", "template cpp:20-gnu14: the stub gets ACCEPTED at 100, not a wrong answer"},
		"a footer without main, for the stub": {func(problem *Problem) {
			problem.Templates[0].Footer = "templates/cpp-footer-no-main.cpp"
			problem.Solutions = nil
		}, "EO823", "template cpp:20-gnu14: the stub does not compile inside the template: "},
		"a footer without main, for a whole program": {func(problem *Problem) {
			problem.Templates[0].Footer = "templates/cpp-footer-no-main.cpp"
			problem.Solutions = nil
		}, "EO824", "template cpp:20-gnu14: a whole program, with its own main(), compiles inside the template"},
	} {
		said := messagesOf(checkedTemplate(t, want.change), want.code)
		if !withPrefix(said, want.said) {
			t.Errorf("%s: %s said %v, not %q", name, want.code, said, want.said)
		}
	}
	found := checkedTemplate(t, func(*Problem) {})
	for _, code := range []string{"EO913", "EO823", "EO824"} {
		if said := messagesOf(found, code); len(said) > 0 {
			t.Errorf("the practice templates raised %s: %v", code, said)
		}
	}
}

func TestStressRunsAFunctionProblemsSolutionsInTheirTemplate(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	code, out, errs := invoke("stress", "testdata/function", "--gen", "gen", "--args", "-n=[2..8]", "--reference", "main",
		"--solution", "sorted", "--iterations", "20")
	if code != 0 || !strings.Contains(out, "stress: gen -n=[2..8] against main, comparing sorted") ||
		!strings.Contains(out, "20 iteration") {
		t.Errorf("stress exited %d, printed %q, said %q", code, out, errs)
	}
}

func TestTwoTemplatesAroundOneSourceAreTwoBuilds(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	dir := t.TempDir()
	for name, body := range map[string]string{
		"problem.json": `{"type": "FUNCTION",
			"templates": [{"runtime": "cpp:20-gnu14", "header": "one.cpp", "footer": "main.cpp"},
				{"runtime": "cpp:20-other", "header": "two.cpp", "footer": "main.cpp"}],
			"solutions": [{"name": "a", "source": "f.cpp", "runtime": "cpp:20-gnu14"},
				{"name": "b", "source": "f.cpp", "runtime": "cpp:20-other"}]}`,
		"one.cpp":  "int base() { return 1; }\n",
		"two.cpp":  "int base() { return 2; }\n",
		"f.cpp":    "int f() { return base(); }\n",
		"main.cpp": "\nint main() { return f(); }\n",
	} {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	shop := workshop(t, dir)
	if err := shop.BuildAll(context.Background(), shop.Problem.Solutions); err != nil {
		t.Fatal(err)
	}
	for name, want := range map[string]int{"a": 1, "b": 2} {
		status, err := run(context.Background(), shop.Programs[solutionName(name)].Exe, Invocation{Dir: dir})
		if err != nil || status.ExitCode != want {
			t.Errorf("solution %s exited %v (%v), not %d", name, status, err, want)
		}
	}
}

func TestCheckReadsTheShapeOfAFunctionProblemsCppTemplates(t *testing.T) {
	t.Parallel()
	ends := "#include <vector>\n#line 1 \"solution.cpp\"\n"
	starts := "\n#line 1 \"grader.cpp\"\nint main() {}\n"
	for name, want := range map[string]struct {
		header, footer, said string
	}{
		"the practice template":       {ends, starts, ""},
		"CRLF line breaks":            {strings.ReplaceAll(ends, "\n", "\r\n"), strings.ReplaceAll(starts, "\n", "\r\n"), ""},
		"an empty header":             {"", starts, ""},
		"a header without #line":      {"#include <vector>\n", starts, "the header's last line is not a #line directive"},
		"blank lines after the #line": {ends + "\n\n", starts, "the header's last line is not a #line directive"},
		"no line break after the #line": {strings.TrimSuffix(ends, "\n"), starts,
			"the header does not end with a line break, so a submission's first line joins its last"},
		"a footer glued to the solution": {ends, strings.TrimPrefix(starts, "\n"), "the footer does not start with a line break"},
	} {
		dir := t.TempDir()
		for file, body := range map[string]string{"h.cpp": want.header, "f.cpp": want.footer} {
			if err := os.WriteFile(filepath.Join(dir, file), []byte(body), 0o644); err != nil {
				t.Fatal(err)
			}
		}
		problem := &Problem{Type: "FUNCTION", dir: dir, Templates: []*Template{
			{Runtime: "cpp:20-gnu14", Header: "h.cpp", Footer: "f.cpp"},
			{Runtime: "python:3.14-python", Header: "f.cpp", Footer: "h.cpp"}}}
		var found Findings
		templateShapeChecks(problem, &found)
		said := messagesOf(found, "EO913")
		switch {
		case want.said == "" && len(said) > 0:
			t.Errorf("%s: EO913 said %v", name, said)
		case want.said != "" && (len(said) != 1 || !said["template cpp:20-gnu14: "+want.said]):
			t.Errorf("%s: EO913 said %v, not %q", name, said, want.said)
		}
	}
}
