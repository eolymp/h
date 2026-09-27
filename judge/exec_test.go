package main

import (
	"context"
	"os"
	"path/filepath"
	"testing"
)

func TestAnAttachedHeaderIsFoundWithAngleBrackets(t *testing.T) {
	needsACompiler(t)
	dir := t.TempDir()
	write := func(name, body string) {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	write("attached_helper.h", "inline int answer() { return 42; }\n")
	write("checker.cpp", "#include <attached_helper.h>\nint main() { return answer() == 42 ? 0 : 1; }\n")
	problem := &Problem{dir: dir}
	built, err := build(problem, "checker", &Program{Source: "checker.cpp", Files: []string{"attached_helper.h"}},
		t.TempDir())
	if err != nil {
		t.Fatal(err)
	}
	if _, err := os.Stat(built.Exe); err != nil {
		t.Error(err)
	}
}

func TestTheSystemCopyOfAHeaderWinsOverAnAttachedOne(t *testing.T) {
	needsACompiler(t)
	dir := t.TempDir()
	system := t.TempDir()
	write := func(name, body string) {
		if err := os.WriteFile(name, []byte(body), 0o644); err != nil {
			t.Fatal(err)
		}
	}
	write(filepath.Join(system, "attached_helper.h"), "inline int answer() { return 7; }\n")
	write(filepath.Join(dir, "attached_helper.h"), "inline int answer() { return 42; }\n")
	write(filepath.Join(dir, "checker.cpp"), "#include <attached_helper.h>\nint main() { return answer(); }\n")
	t.Setenv("CPLUS_INCLUDE_PATH", system)
	problem := &Problem{dir: dir}
	built, err := build(problem, "checker", &Program{Source: "checker.cpp", Files: []string{"attached_helper.h"}},
		t.TempDir())
	if err != nil {
		t.Fatal(err)
	}
	status, err := run(context.Background(), built.Exe, Invocation{Dir: built.Dir, LimitMS: 5000})
	if err != nil {
		t.Fatal(err)
	}
	if status.ExitCode != 7 {
		t.Errorf("the program used the attached header, exiting %d", status.ExitCode)
	}
}
