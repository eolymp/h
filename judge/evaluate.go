package main

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"maps"
	"os"
	"path/filepath"
	"strings"
)

type Attempt struct {
	Name    string
	Verdict Verdict
	Score   Points
	Groups  []*GroupResult
}

func (w *Workspace) Evaluate(ctx context.Context, solution *Solution) (*Attempt, error) {
	name := solution.Name
	var built *Built
	var err error
	if !w.Problem.Output() {
		if built, err = w.Build(ctx, solutionName(name), w.Problem.programOf(solution)); err != nil {
			return nil, err
		}
	}

	checker, err := w.Build(ctx, "checker", w.Problem.Checker)
	if err != nil {
		return nil, err
	}

	var interactor *Built
	if w.Problem.Interactive() {
		if interactor, err = w.Build(ctx, "interactor", w.Problem.Interactor); err != nil {
			return nil, err
		}
	}

	gate := admissionFor(w.Problem)
	results := map[string]*RunResult{}

	waiting := w.plan()
	for len(waiting) > 0 {
		var blocked []*Planned
		for _, one := range waiting {
			switch gate.Admit(one) {
			case Blocked:
				blocked = append(blocked, one)
				continue
			case Rejected:
				results[reference(one)] = &RunResult{Group: one.Group, Index: one.Test.Index,
					Cost: Points(one.Test.Score), Verdict: Skipped}
				continue
			}

			result, err := w.judge(ctx, one, solution, built, checker, interactor)
			if err != nil {
				return nil, err
			}
			results[reference(one)] = result
			gate.Notify(one, result)
		}

		if len(blocked) == len(waiting) {
			for _, one := range blocked {
				results[reference(one)] = &RunResult{Group: one.Group, Index: one.Test.Index,
					Cost: Points(one.Test.Score), Verdict: Skipped}
			}
			break
		}
		waiting = blocked
	}

	attempt := &Attempt{Name: name}
	for _, testset := range w.Problem.Testsets {
		var runs []*RunResult
		for _, test := range testset.Tests {
			if found, known := results[reference(&Planned{Group: testset.Index, Test: test})]; known {
				runs = append(runs, found)
			}
		}
		attempt.Groups = append(attempt.Groups, summarizeGroup(testset.Index, testset.ScoringMode, runs))
	}
	attempt.Verdict, attempt.Score = summarizeSubmission(attempt.Groups)
	return attempt, nil
}

func (w *Workspace) plan() []*Planned {
	var out []*Planned
	for _, testset := range w.Problem.Testsets {
		for _, test := range testset.Tests {
			out = append(out, &Planned{Group: testset.Index, Test: test})
		}
	}
	return out
}

type trial struct {
	made  *Prepared
	limit int
	env   map[string]string
	work  string
}

func (w *Workspace) judge(ctx context.Context, one *Planned, solution *Solution, built, checker,
	interactor *Built) (*RunResult, error) {
	testset := w.Problem.Testset(one.Group)
	at := trial{made: w.Tests[reference(one)], limit: testset.Limit(w.Problem), env: w.metadata(one),
		work: filepath.Join(w.Dir, "runs", solutionName(solution.Name),
			fmt.Sprintf("%d-%d", one.Group, one.Test.Index))}
	result := &RunResult{Group: one.Group, Index: one.Test.Index, Cost: Points(one.Test.Score)}
	if built == nil {
		return w.upload(ctx, at, solution.uploaded[reference(one)], checker, result)
	}
	return w.try(ctx, at, built, checker, interactor, result)
}

func (w *Workspace) upload(ctx context.Context, at trial, given string, checker *Built,
	result *RunResult) (*RunResult, error) {
	if err := os.MkdirAll(at.work, 0o755); err != nil {
		return nil, err
	}
	body := []byte{}
	if given != "" {
		var err error
		if body, err = os.ReadFile(w.Problem.Path(given)); err != nil {
			return nil, fmt.Errorf("the output for test %d:%d: %w", result.Group, result.Index, err)
		}
	}
	output := filepath.Join(at.work, "output.txt")
	if err := os.WriteFile(output, body, 0o644); err != nil {
		return nil, err
	}
	if _, err := w.check(ctx, at.env, at.made, checker, at.work, output, result); err != nil {
		return nil, err
	}
	if given == "" {
		result.Message = "no file was given for this test, so it was judged as an empty one: " + result.Message
	}
	return result, nil
}

func (w *Workspace) try(ctx context.Context, at trial, solution, checker, interactor *Built,
	result *RunResult) (*RunResult, error) {
	made, limit, work := at.made, at.limit, at.work
	if err := os.MkdirAll(work, 0o755); err != nil {
		return nil, err
	}
	output := filepath.Join(work, "output.txt")

	var status, jury *Status
	var err error
	if interactor != nil {
		status, jury, result.Transcript, err = w.interact(ctx, at.env, made, solution, interactor, work, output, limit)
	} else {
		status, err = w.batch(ctx, made, solution, work, output, limit)
	}
	if err != nil {
		return nil, err
	}
	if err := made.intact(solution.Name); err != nil {
		return nil, err
	}

	result.Wall = status.Wall

	verdict := Accepted
	switch {
	case status.TimedOut:
		verdict = TimeLimit
	case status.Signal || status.ExitCode != 0:
		verdict = RuntimeFail
		result.Message = fmt.Sprintf("exit %d", status.ExitCode)
	}

	if verdict != Accepted {
		result.Verdict = verdict
		if jury != nil {
			said, _ := os.ReadFile(filepath.Join(work, "interactor.log"))
			if line := firstLine(string(said)); line != "" {
				result.Message = strings.TrimPrefix(result.Message+"; the interactor said: "+line, "; ")
			}
		}
		return result, nil
	}

	if jury != nil {
		said, _ := os.ReadFile(filepath.Join(work, "interactor.log"))
		result.Warnings = warningsIn("interactor", string(said))
		switch jury.ExitCode {
		case 0:
		case 1, 2:
			result.Verdict = WrongAnswer
			result.Message = firstLine(string(said))
			return result, nil
		default:
			result.Verdict = Failure
			result.Message = firstLine(string(said))
			return result, nil
		}
	}

	return w.check(ctx, at.env, made, checker, work, output, result)
}

func (w *Workspace) batch(ctx context.Context, made *Prepared, solution *Built, work, output string, limit int) (*Status, error) {
	input, err := os.Open(made.Input)
	if err != nil {
		return nil, err
	}
	defer input.Close()

	file, err := os.Create(output)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	alone, err := os.MkdirTemp(w.Temp, "eo-judge-run-")
	if err != nil {
		return nil, err
	}
	defer os.RemoveAll(alone)
	return run(ctx, solution.Exe, Invocation{Dir: alone, Stdin: input, Stdout: file, LimitMS: limit})
}

func (w *Workspace) check(ctx context.Context, env map[string]string, made *Prepared, checker *Built,
	work, output string, result *RunResult) (*RunResult, error) {
	status, said, err := runChecker(ctx, checker, made, output, work, env)
	if err != nil {
		return nil, err
	}

	result.Message = firstLine(string(said))
	result.Warnings = warningsIn("checker", string(said))
	checked(result, status, said)
	return result, nil
}

func checked(result *RunResult, status *Status, said []byte) {
	switch status.ExitCode {
	case 0:
		result.Verdict = Accepted
	case 1, 2:
		result.Verdict = WrongAnswer
	case 7:
		points, err := readPoints(bytes.NewReader(said))
		if err != nil {
			result.Verdict = Failure
			result.Message = err.Error()
			return
		}
		result.Fraction = points
		result.Verdict = verdictOfPoints(points, result.Cost)
	default:
		result.Verdict = Failure
	}

	result.Score = scoreOfRun(result.Verdict, result.Cost, result.Fraction)
	if result.Verdict == Accepted {
		result.Fraction = result.Cost
	}
}

func runChecker(ctx context.Context, checker *Built, made *Prepared, output, work string,
	metadata map[string]string) (*Status, []byte, error) {
	log := filepath.Join(work, "checker.log")
	file, err := os.Create(log)
	if err != nil {
		return nil, nil, err
	}
	env := maps.Clone(metadata)
	env["INPUT_FILE"], env["OUTPUT_FILE"], env["ANSWER_FILE"] = made.Input, output, made.Answer
	status, err := run(ctx, checker.Exe, Invocation{
		Args: []string{made.Input, output, made.Answer}, Dir: work, Stdout: file, Stderr: file,
		LimitMS: checkerLimit, Env: env,
	})
	file.Close()
	if err != nil {
		return nil, nil, err
	}
	said, _ := os.ReadFile(log)
	return status, said, nil
}

func (w *Workspace) interact(ctx context.Context, env map[string]string, made *Prepared, solution,
	interactor *Built, work, output string, limit int) (*Status, *Status, []string, error) {
	input := made.Input
	var last, lastJury *Status
	var transcript []string

	for phase := 1; phase <= w.Problem.RunCount; phase++ {
		summary := output
		if phase < w.Problem.RunCount {
			summary = filepath.Join(work, fmt.Sprintf("handoff-%d.txt", phase))
		}

		var record *dialogue
		if w.transcript {
			record = &dialogue{}
		}
		status, jury, err := w.onePhase(ctx, input, summary, solution, interactor, work, limit,
			env, w.answerFor(made), record)
		if err != nil {
			return nil, nil, nil, err
		}
		if record != nil {
			if w.Problem.RunCount > 1 {
				transcript = append(transcript, fmt.Sprintf("phase %d", phase))
			}
			transcript = append(transcript, record.finished()...)
		}
		last, lastJury = status, jury
		if status.TimedOut || status.Signal || status.ExitCode != 0 || jury.ExitCode != 0 {
			return status, jury, transcript, nil
		}
		input = summary
	}
	return last, lastJury, transcript, nil
}

func (w *Workspace) metadata(one *Planned) map[string]string {
	if one == nil {
		return map[string]string{"EOLYMP": "1"}
	}
	return map[string]string{
		"EOLYMP": "1", "TEST_ID": reference(one), "TEST_COST": fmt.Sprint(one.Test.Score),
		"TEST_INDEX": fmt.Sprint(one.Test.Index), "TEST_GROUP": fmt.Sprint(one.Group),
	}
}

func (w *Workspace) answerFor(made *Prepared) string {
	if made == nil || (made.Test.Answer == "" && made.Test.AnswerGenerator == "") {
		return ""
	}
	return made.Answer
}

func (w *Workspace) onePhase(ctx context.Context, input, summary string, solution, interactor *Built,
	work string, limit int, env map[string]string, answer string, record *dialogue) (*Status, *Status, error) {
	toJury, fromPlayer, err := os.Pipe()
	if err != nil {
		return nil, nil, err
	}
	toPlayer, fromJury, err := os.Pipe()
	if err != nil {
		return nil, nil, err
	}

	juryLog, err := os.Create(filepath.Join(work, "interactor.log"))
	if err != nil {
		return nil, nil, err
	}
	defer juryLog.Close()

	arguments := []string{input, summary}
	if answer != "" {
		arguments = append(arguments, answer)
	}

	alone, err := os.MkdirTemp(w.Temp, "eo-judge-run-")
	if err != nil {
		return nil, nil, err
	}
	defer os.RemoveAll(alone)

	playerOut, juryOut := fromPlayer, fromJury
	var heard, answered *os.File
	if record != nil {
		var said, told *os.File
		if heard, said, err = os.Pipe(); err != nil {
			return nil, nil, err
		}
		if answered, told, err = os.Pipe(); err != nil {
			heard.Close()
			said.Close()
			return nil, nil, err
		}
		record.relay("solution:   ", heard, fromPlayer)
		record.relay("interactor: ", answered, fromJury)
		playerOut, juryOut = said, told
	}

	done := make(chan *Status, 1)
	go func() {
		status, _ := run(ctx, interactor.Exe, Invocation{
			Args: arguments, Dir: work, Stdin: toJury, Stdout: juryOut, Stderr: juryLog,
			LimitMS: limit + 1000,
			Env:     env,
		})
		toJury.Close()
		juryOut.Close()
		if heard != nil {
			heard.Close()
		}
		done <- status
	}()

	status, err := run(ctx, solution.Exe, Invocation{
		Dir: alone, Stdin: toPlayer, Stdout: playerOut, Stderr: io.Discard, LimitMS: limit,
	})
	toPlayer.Close()
	playerOut.Close()
	if answered != nil {
		answered.Close()
	}
	jury := <-done
	if record != nil {
		record.finished()
	}

	if err != nil {
		return nil, nil, err
	}
	if jury == nil {
		jury = &Status{}
	}
	return status, jury, nil
}
