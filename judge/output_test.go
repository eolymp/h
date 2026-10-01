package main

import (
	"context"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestAnOutputProblemsSolutionsAreReadStrictly(t *testing.T) {
	t.Parallel()
	tests := `"testsets": [{"index": 1, "tests": [{"index": 1}, {"index": 2}]},
		{"index": 2, "tests": [{"index": 1}]}]`
	for body, want := range map[string]string{
		`{"type": "OUTPUT", "solutions": [{"name": "a", "source": "a.cpp"}], ` + tests + `}`:                         `solution "a" of an OUTPUT problem gives its answer files in outputs, not a source`,
		`{"type": "OUTPUT", "solutions": [{"name": "a"}], ` + tests + `}`:                                            `solution "a" of an OUTPUT problem gives its answer files in outputs, not a source`,
		`{"solutions": [{"name": "a", "source": "a.cpp", "outputs": {"1": "a.txt"}}]}`:                               `solution "a" has outputs, which only an OUTPUT problem's solutions give; a PROGRAM problem's solution is a source`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"3": "a.txt"}}], ` + tests + `}`:                 `solution "a" gives an output for test "3", and the problem has no such test`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2:2": "a.txt"}}], ` + tests + `}`:               `solution "a" gives an output for test "2:2", and the problem has no such test`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"x": "a.txt"}}], ` + tests + `}`:                 `solution "a" gives an output for test "x", and the problem has no such test`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"1": "a.txt"}}], ` + tests + `}`:                 `solution "a" gives an output for test "1", which is test 1 of testsets 1 and 2; write "1:1" or "2:1"`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2": "a.txt", "1:2": "b.txt"}}], ` + tests + `}`: `solution "a" gives two outputs for test 1:2, as "1:2" and "2"`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2": ""}}], ` + tests + `}`:                      `solution "a" gives an empty file name for test "2"`,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2": "nope.txt"}}], ` + tests + `}`:              `solution "a" gives nope.txt for test 1:2, and it cannot be read: `,
		`{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2": "."}}], ` + tests + `}`:                     `solution "a" gives . for test 1:2, and it cannot be read: it is a directory, not a file`,
	} {
		err := loading(t, body)
		if err == nil || !strings.Contains(err.Error(), want) {
			t.Errorf("%s gave %v, not %q", body, err, want)
		}
	}
	dir := t.TempDir()
	for name, body := range map[string]string{"two.txt": "2\n", "other.txt": "1\n",
		"problem.json": `{"type": "OUTPUT", "solutions": [{"name": "a", "outputs": {"2": "two.txt", "2:1": "other.txt"}}], ` + tests + `}`} {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	problem, err := LoadProblem(dir)
	if err != nil {
		t.Fatal(err)
	}
	if got := problem.Solutions[0].uploaded; len(got) != 2 || got["1:2"] != "two.txt" || got["2:1"] != "other.txt" {
		t.Errorf("the outputs resolved to %v", got)
	}
}

func TestRunJudgesTheAnswerFilesOfAnOutputProblem(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	shop := workshop(t, "testdata/output")
	judged := judgeAll(t, shop)
	for name, want := range map[string]struct {
		verdict Verdict
		score   Points
	}{
		"full":     {Accepted, 100},
		"partial":  {WrongAnswer, 60},
		"attacked": {WrongAnswer, 70},
	} {
		got := judged[name]
		if got == nil {
			t.Fatalf("%s was never judged", name)
		}
		if got.Verdict != want.verdict || got.Score != want.score {
			t.Errorf("%s scored %v at %v, not %v at %v", name, got.Verdict, got.Score, want.verdict, want.score)
		}
	}
	missing := judged["partial"].Groups[0].Runs[2]
	if missing.Verdict != WrongAnswer || !strings.HasPrefix(missing.Message, "no file was given for this test, so it was judged as an empty one: ") {
		t.Errorf("the test partial gives no file for is %v: %q", missing.Verdict, missing.Message)
	}
	attacked := judged["attacked"].Groups[0].Runs[0]
	if !strings.Contains(attacked.Message, "the queen in row 2, column 2 is attacked") {
		t.Errorf("the attacked board says %q", attacked.Message)
	}
	for _, group := range judged["full"].Groups {
		for _, one := range group.Runs {
			if len(one.Warnings) > 0 {
				t.Errorf("test %d:%d warned %v; c.output_only covers the unread answer", one.Group, one.Index, one.Warnings)
			}
		}
	}
}

func TestRunAndCheckTakeAnOutputProblemAndStressRefusesIt(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	code, out, errs := invoke("run", "testdata/output")
	if code != 0 || !strings.Contains(out, "full: ACCEPTED, 100") || !strings.Contains(out, "partial: WRONG_ANSWER, 60") {
		t.Errorf("run exited %d, printed %q, said %q", code, out, errs)
	}
	code, out, errs = invoke("check", "testdata/output")
	if code != 0 || strings.Contains(out, "EO819") || strings.Contains(out, "EO801") {
		t.Errorf("check exited %d, printed %q, said %q", code, out, errs)
	}
	code, out, errs = invoke("stress", "testdata/output", "--args", "4")
	if code != 3 || out != "" || !strings.Contains(errs, "eo-judge stress runs PROGRAM and FUNCTION problems only, and this one is OUTPUT") {
		t.Errorf("stress exited %d, printed %q, said %q", code, out, errs)
	}
}

func checkedWith(t *testing.T, checker string) Findings {
	t.Helper()
	shop := workshop(t, "testdata/output")
	shop.Problem.Checker = &Program{Source: checker}
	found, err := shop.Check(context.Background(), false)
	if err != nil {
		t.Fatal(err)
	}
	return found
}

func TestCheckFindsAnOutputCheckerThatTakesAnotherTestsAnswer(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	if said := messagesOf(checkedWith(t, "checker.cpp"), "EO822"); len(said) > 0 {
		t.Errorf("a checker that reads n warned EO822: %v", said)
	}
	said := messagesOf(checkedWith(t, "blind.cpp"), "EO822")
	for _, want := range []string{
		"test 1:1: the checker accepts the answer of test 1:2 as this test's output",
		"test 1:2: the checker accepts the answer of test 1:3 as this test's output",
		"test 1:3: the checker accepts the answer of test 1:1 as this test's output",
	} {
		if !said[want] {
			t.Errorf("EO822 did not say %q: %v", want, said)
		}
	}
}

func TestCheckLooksForAnEmptyOutputOnEveryTestOfAnOutputProblem(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	said := messagesOf(checkedWith(t, "lazy.cpp"), "EO802")
	if len(said) != 1 || !said["test 1:3: the checker accepts an empty output"] {
		t.Errorf("EO802 said %v", said)
	}
}

func writeProblem(t *testing.T, files map[string]string) string {
	t.Helper()
	dir := t.TempDir()
	for name, body := range files {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	return dir
}

func TestCheckTriesAnotherTestsAnswerOnlyWhereTheInputsDiffer(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	blind, err := filepath.Abs("testdata/output/blind.cpp")
	if err != nil {
		t.Fatal(err)
	}
	board := ".Q..\n...Q\nQ...\n..Q.\n"
	for name, tests := range map[string]string{
		"two tests with the same input": `{"index": 1, "score": 50, "input": "in.txt", "answer": "ans.txt"},
			{"index": 2, "score": 50, "input": "same.txt", "answer": "ans.txt"}`,
		"one test": `{"index": 1, "score": 100, "input": "in.txt", "answer": "ans.txt"}`,
		"no test":  ``,
	} {
		dir := writeProblem(t, map[string]string{
			"problem.json": `{"type": "OUTPUT", "checker": {"source": "` + blind + `"},
				"solutions": [{"name": "full", "type": "CORRECT", "outputs": {}}],
				"testsets": [{"index": 1, "tests": [` + tests + `]}]}`,
			"in.txt": "4\n", "same.txt": "4\n", "ans.txt": board,
		})
		code, out, errs := invoke("check", dir)
		if code != 0 || strings.Contains(out, "EO822") {
			t.Errorf("%s: check exited %d, printed %q, said %q", name, code, out, errs)
		}
	}
}
