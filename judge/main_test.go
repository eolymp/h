package main

import (
	"bytes"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func invoke(args ...string) (int, string, string) {
	var out, errs bytes.Buffer
	code := realMain(args, &out, &errs)
	return code, out.String(), errs.String()
}

func TestNoCommandPrintsTheUsage(t *testing.T) {
	code, _, errs := invoke()
	if code != 2 || !strings.Contains(errs, "eo-judge run <problem>") {
		t.Errorf("exit %d, said %q", code, errs)
	}
	code, _, errs = invoke("fly", "testdata/broken")
	if code != 2 || !strings.Contains(errs, "eo-judge run <problem>") {
		t.Errorf("an unknown command exited %d, said %q", code, errs)
	}
	code, _, _ = invoke("run")
	if code != 2 {
		t.Errorf("a command without a problem exited %d", code)
	}
}

func TestAProblemThatCannotBeReadIsAnError(t *testing.T) {
	code, _, errs := invoke("lint", t.TempDir())
	if code != 3 || !strings.Contains(errs, "problem.json") {
		t.Errorf("exit %d, said %q", code, errs)
	}
}

func TestLintReportsToTheGivenWriter(t *testing.T) {
	code, out, _ := invoke("lint", "testdata/broken")
	if code != 0 || !strings.Contains(out, "eo-judge:") {
		t.Errorf("exit %d, printed %q", code, out)
	}
	code, _, _ = invoke("lint", "--strict", "testdata/broken")
	if code != 1 {
		t.Errorf("a strict lint with warnings exited %d", code)
	}
}

func uncompilable(t *testing.T) string {
	t.Helper()
	dir := t.TempDir()
	write := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	write("problem.json", `{"type": "PROGRAM", "checker": {"source": "checker.cpp"}}`)
	write("checker.cpp", "this is not C++\n")
	return dir
}

func TestTheWorkspaceIsRemovedWhenARunFails(t *testing.T) {
	needsACompiler(t)
	temporary := t.TempDir()
	t.Setenv("TMPDIR", temporary)
	code, _, errs := invoke("run", uncompilable(t))
	if code != 3 || !strings.Contains(errs, "does not compile") {
		t.Fatalf("exit %d, said %q", code, errs)
	}
	left, err := os.ReadDir(temporary)
	if err != nil {
		t.Fatal(err)
	}
	if len(left) != 0 {
		t.Errorf("the run left %d entries in the temporary directory, such as %s", len(left), left[0].Name())
	}
}

func TestAWorkspaceThatWasAskedForIsKept(t *testing.T) {
	needsACompiler(t)
	kept := filepath.Join(t.TempDir(), "work")
	code, _, _ := invoke("check", "--work", kept, uncompilable(t))
	if code != 3 {
		t.Fatalf("exit %d", code)
	}
	if _, err := os.Stat(filepath.Join(kept, "checker")); err != nil {
		t.Errorf("the workspace was not kept: %v", err)
	}
}
