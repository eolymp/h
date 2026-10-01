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
