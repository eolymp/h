package main

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strings"
)

var lineDirective = regexp.MustCompile(`^\s*#\s*line\b`)

func templateShapeChecks(problem *Problem, found *Findings) {
	if !problem.Function() {
		return
	}
	for _, template := range problem.Templates {
		if !template.cpp() {
			continue
		}
		where := "template " + template.Runtime
		if header := templatePart(problem, template.Header); len(header) > 0 {
			text := strings.TrimSuffix(strings.TrimSuffix(string(header), "\n"), "\r")
			lines := strings.Split(text, "\n")
			switch {
			case header[len(header)-1] != '\n':
				found.warn("EO913", where, "the header does not end with a line break, so a submission's first "+
					"line joins its last", `end the header with #line 1 "solution.cpp" and a line break`)
			case !lineDirective.MatchString(lines[len(lines)-1]):
				found.warn("EO913", where, "the header's last line is not a #line directive",
					`end it with #line 1 "solution.cpp", with nothing after it, so a compilation error or a `+
						"warning counts the contestant's own lines")
			}
		}
		if footer := templatePart(problem, template.Footer); len(footer) > 0 && footer[0] != '\n' &&
			!strings.HasPrefix(string(footer), "\r\n") {
			found.warn("EO913", where, "the footer does not start with a line break",
				"a submission that does not end in one runs into the footer's first line; start the footer "+
					"with an empty line")
		}
	}
}

func templatePart(problem *Problem, name string) []byte {
	if name == "" {
		return nil
	}
	body, _ := os.ReadFile(problem.Path(name))
	return body
}

const wholeProgram = "int main() { return 0; }\n"

func (w *Workspace) templateChecks(ctx context.Context, found *Findings) error {
	if !w.Problem.Function() {
		return nil
	}
	for _, template := range w.Problem.Templates {
		if !template.cpp() {
			continue
		}
		where := "template " + template.Runtime
		if template.Source != "" {
			if err := w.stubCheck(ctx, template, where, found); err != nil {
				return err
			}
		}
		whole := filepath.Join(w.Dir, "whole-program.cpp")
		if err := os.WriteFile(whole, []byte(wholeProgram), 0o644); err != nil {
			return err
		}
		_, err := w.tools.build(ctx, w.Problem, "whole-program."+template.Runtime,
			&Program{Source: whole, Runtime: template.Runtime, wrapped: template}, w.Dir)
		var refused *notCompiled
		switch {
		case err == nil:
			found.warn("EO824", where, "a whole program, with its own main(), compiles inside the template",
				"give the template main(), in its footer or its header, so a submission that brings its own "+
					"gets a compilation error, as it does on the judge")
		case !errors.As(err, &refused):
			return err
		}
	}
	return nil
}

func (w *Workspace) stubCheck(ctx context.Context, template *Template, where string, found *Findings) error {
	stub := &Solution{Name: "stub." + template.Runtime, Source: template.Source, Runtime: template.Runtime,
		template: template}
	if _, err := w.Build(ctx, solutionName(stub.Name), w.Problem.programOf(stub)); err != nil {
		var refused *notCompiled
		if !errors.As(err, &refused) {
			return err
		}
		found.warn("EO823", where, "the stub does not compile inside the template: "+firstLine(refused.said),
			"a contestant starts from the stub; make header, stub and footer compile together")
		return nil
	}
	attempt, err := w.Evaluate(ctx, stub)
	if err != nil {
		return err
	}
	if attempt.Verdict != WrongAnswer {
		found.warn("EO823", where, fmt.Sprintf("the stub gets %s at %g, not a wrong answer", attempt.Verdict,
			attempt.Score),
			"a contestant who submits the template unchanged should get a wrong answer; return something "+
				"the checker refuses")
	}
	return nil
}
