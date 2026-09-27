package main

import (
	"bytes"
	"context"
	"crypto/sha1"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
)

type Prepared struct {
	Group    int
	Test     *Test
	Input    string
	Answer   string
	Valid    bool
	Broken   bool
	Why      string
	Warnings []Warning

	sums [2][sha256.Size]byte
}

func (p *Prepared) files() [2]string { return [2]string{p.Input, p.Answer} }

func (p *Prepared) seal() error {
	for at, path := range p.files() {
		body, err := os.ReadFile(path)
		if err != nil {
			return err
		}
		p.sums[at] = sha256.Sum256(body)
		if err := os.Chmod(path, 0o444); err != nil {
			return err
		}
	}
	return nil
}

func (p *Prepared) intact(during string) error {
	for at, path := range p.files() {
		body, err := os.ReadFile(path)
		if err != nil || sha256.Sum256(body) != p.sums[at] {
			return fmt.Errorf("%s changed while %s ran; a program that writes into eo-judge's workspace "+
				"gets no score, and the run stops here", path, during)
		}
	}
	return nil
}

type Workspace struct {
	Problem  *Problem
	Dir      string
	Programs map[string]*Built
	Tests    map[string]*Prepared
	Warnings []Warning
}

func normalise(body []byte) []byte {
	return bytes.ReplaceAll(body, []byte("\r\n"), []byte("\n"))
}

func keyOf(parts ...string) string {
	sum := sha1.New()
	for _, one := range parts {
		sum.Write([]byte(one))
		sum.Write([]byte{0})
	}
	return hex.EncodeToString(sum.Sum(nil))[:16]
}

func NewWorkspace(problem *Problem, dir string) *Workspace {
	return &Workspace{Problem: problem, Dir: dir,
		Programs: map[string]*Built{}, Tests: map[string]*Prepared{}}
}

func (w *Workspace) Build(name string, program *Program) (*Built, error) {
	if made, known := w.Programs[name]; known {
		return made, nil
	}
	made, err := build(w.Problem, name, program, w.Dir)
	if err != nil {
		return nil, err
	}
	w.Programs[name] = made
	return made, nil
}

func (w *Workspace) BuildAll() error {
	problem := w.Problem
	if problem.Checker != nil {
		if _, err := w.Build("checker", problem.Checker); err != nil {
			return err
		}
	}
	if problem.Validator != nil {
		if _, err := w.Build("validator", problem.Validator); err != nil {
			return err
		}
	}
	if problem.Interactor != nil {
		if _, err := w.Build("interactor", problem.Interactor); err != nil {
			return err
		}
	}
	for name, script := range problem.Scripts {
		if _, err := w.Build("script."+name, script); err != nil {
			return err
		}
	}
	return nil
}

func (w *Workspace) script(name string) (*Built, error) {
	script, known := w.Problem.Scripts[name]
	if !known {
		return nil, fmt.Errorf("no script named %q", name)
	}
	return w.Build("script."+name, script)
}

func (w *Workspace) Generate(ctx context.Context) error {
	tests := filepath.Join(w.Dir, "tests")
	if err := os.RemoveAll(tests); err != nil {
		return err
	}
	if err := os.MkdirAll(tests, 0o755); err != nil {
		return err
	}

	for _, testset := range w.Problem.Testsets {
		for _, test := range testset.Tests {
			made := &Prepared{Group: testset.Index, Test: test}
			name := fmt.Sprintf("%02d-%03d", testset.Index, test.Index)
			made.Input = filepath.Join(tests, name+".in")
			made.Answer = filepath.Join(tests, name+".ans")

			if err := w.makeInput(ctx, made); err != nil {
				return err
			}
			if err := w.makeAnswer(ctx, made); err != nil {
				return err
			}
			if err := made.seal(); err != nil {
				return err
			}
			w.Tests[reference(&Planned{Group: testset.Index, Test: test})] = made
		}
	}
	return nil
}

func (w *Workspace) makeInput(ctx context.Context, made *Prepared) error {
	test := made.Test
	if test.Input != "" {
		body, err := os.ReadFile(w.Problem.Path(test.Input))
		if err != nil {
			return err
		}
		return os.WriteFile(made.Input, normalise(body), 0o644)
	}

	if test.Generator == nil {
		return fmt.Errorf("test %d:%d has neither an input nor a generator", made.Group, test.Index)
	}

	built, err := w.script(test.Generator.Script)
	if err != nil {
		return err
	}

	file, err := os.Create(made.Input)
	if err != nil {
		return err
	}
	defer file.Close()

	status, err := run(ctx, built.Exe, Invocation{
		Args: test.Generator.Arguments, Dir: built.Dir, Stdout: file, LimitMS: 60000,
		Env: map[string]string{"EOLYMP": "1"},
	})
	if err != nil {
		return fmt.Errorf("generator %s: %w", test.Generator.Script, err)
	}
	if status.ExitCode != 0 {
		return fmt.Errorf("generator %s exited %d: %s", test.Generator.Script, status.ExitCode,
			bytes.TrimSpace(status.Stderr))
	}
	made.Warnings = append(made.Warnings, warningsIn(test.Generator.Script, string(status.Stderr))...)
	return nil
}

func (w *Workspace) makeAnswer(ctx context.Context, made *Prepared) error {
	test := made.Test

	if test.Answer != "" {
		body, err := os.ReadFile(w.Problem.Path(test.Answer))
		if err != nil {
			return err
		}
		return os.WriteFile(made.Answer, normalise(body), 0o644)
	}

	if test.AnswerGenerator == "" {
		if !w.Problem.Interactive() {
			return fmt.Errorf("test %d:%d has no answer and no answerGenerator", made.Group, test.Index)
		}
		body, err := os.ReadFile(made.Input)
		if err != nil {
			return err
		}
		return os.WriteFile(made.Answer, body, 0o644)
	}

	built, err := w.script(test.AnswerGenerator)
	if err != nil {
		return err
	}

	input, err := os.Open(made.Input)
	if err != nil {
		return err
	}
	defer input.Close()

	file, err := os.Create(made.Answer)
	if err != nil {
		return err
	}
	defer file.Close()

	status, err := run(ctx, built.Exe, Invocation{
		Dir: built.Dir, Stdin: input, Stdout: file, LimitMS: 60000,
		Env: map[string]string{"EOLYMP": "1"},
	})
	if err != nil {
		return fmt.Errorf("answer generator %s: %w", test.AnswerGenerator, err)
	}
	if status.ExitCode != 0 {
		return fmt.Errorf("answer generator %s exited %d on test %d:%d", test.AnswerGenerator,
			status.ExitCode, made.Group, test.Index)
	}
	return nil
}

func (w *Workspace) Validate(ctx context.Context, group bool) error {
	if w.Problem.Validator == nil {
		return nil
	}
	built, err := w.Build("validator", w.Problem.Validator)
	if err != nil {
		return err
	}

	for _, made := range w.Tests {
		args := []string{made.Input}
		if group {
			args = append(args, "--group", fmt.Sprint(made.Group))
		}
		status, err := run(ctx, built.Exe, Invocation{
			Args: args, Dir: built.Dir, LimitMS: 30000, Env: map[string]string{"EOLYMP": "1"},
		})
		if err != nil {
			return err
		}
		said := string(status.Stdout) + string(status.Stderr)
		made.Valid = status.ExitCode == 0
		made.Broken = status.ExitCode == juryError
		if !made.Valid {
			made.Why = firstLine(said)
		}
		made.Warnings = append(made.Warnings, warningsIn("validator", said)...)
	}
	return nil
}

func (w *Workspace) describe(ctx context.Context, made *Prepared) (string, error) {
	built, err := w.Build("validator", w.Problem.Validator)
	if err != nil {
		return "", err
	}
	status, err := run(ctx, built.Exe, Invocation{
		Args: []string{made.Input, "--group", fmt.Sprint(made.Group), "--eo-describe"},
		Dir:  built.Dir, LimitMS: 30000, Env: map[string]string{"EOLYMP": "1"},
	})
	if err != nil {
		return "", err
	}
	return string(status.Stdout), nil
}

func firstLine(text string) string {
	for _, line := range bytes.Split([]byte(text), []byte("\n")) {
		trimmed := bytes.TrimSpace(line)
		if len(trimmed) > 0 {
			return string(trimmed)
		}
	}
	return ""
}
