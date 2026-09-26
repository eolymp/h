package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"sort"
)

const usage = `eo-judge runs an Eolymp problem the way the judge does.

  eo-judge run <problem> [--solution name]   build, generate, validate, judge, score
  eo-judge check <problem> [--deep]          the whole-problem and configuration checks
  eo-judge lint <problem>                    what the header cannot see

  --strict   make every warning fatal
  --work     keep the workspace in this directory
`

func main() {
	if len(os.Args) < 2 {
		fmt.Fprint(os.Stderr, usage)
		os.Exit(2)
	}

	command := os.Args[1]
	flags := flag.NewFlagSet(command, flag.ExitOnError)
	strict := flags.Bool("strict", false, "make every warning fatal")
	deep := flags.Bool("deep", false, "run the slow hostile outputs")
	only := flags.String("solution", "", "judge one solution by name")
	work := flags.String("work", "", "keep the workspace here")
	flags.Usage = func() { fmt.Fprint(os.Stderr, usage) }
	if err := flags.Parse(os.Args[2:]); err != nil {
		os.Exit(2)
	}

	dir := flags.Arg(0)
	if dir == "" {
		fmt.Fprint(os.Stderr, usage)
		os.Exit(2)
	}

	problem, err := LoadProblem(dir)
	if err != nil {
		fmt.Fprintln(os.Stderr, "eo-judge:", err)
		os.Exit(3)
	}

	if command == "lint" {
		os.Exit(report(Lint(problem), *strict))
	}

	space := *work
	if space == "" {
		space, err = os.MkdirTemp("", "eo-judge-")
		if err != nil {
			fmt.Fprintln(os.Stderr, "eo-judge:", err)
			os.Exit(3)
		}
		defer os.RemoveAll(space)
	} else if err := os.MkdirAll(space, 0o755); err != nil {
		fmt.Fprintln(os.Stderr, "eo-judge:", err)
		os.Exit(3)
	}

	ctx := context.Background()
	shop := NewWorkspace(problem, space)

	switch command {
	case "check":
		found, err := shop.Check(ctx, *deep)
		if err != nil {
			fmt.Fprintln(os.Stderr, "eo-judge:", err)
			os.Exit(3)
		}
		os.Exit(report(append(found, Lint(problem)...), *strict))
	case "run":
		os.Exit(runProblem(ctx, shop, *only, *strict))
	default:
		fmt.Fprint(os.Stderr, usage)
		os.Exit(2)
	}
}

func runProblem(ctx context.Context, shop *Workspace, only string, strict bool) int {
	if err := shop.BuildAll(); err != nil {
		fmt.Fprintln(os.Stderr, "eo-judge:", err)
		return 3
	}
	if err := shop.Generate(ctx); err != nil {
		fmt.Fprintln(os.Stderr, "eo-judge:", err)
		return 3
	}
	if err := shop.Validate(ctx, true); err != nil {
		fmt.Fprintln(os.Stderr, "eo-judge:", err)
		return 3
	}

	var found Findings
	invalid := 0
	for _, made := range shop.sorted() {
		if shop.Problem.Validator != nil && !made.Valid {
			invalid++
			fmt.Printf("test %d:%d is invalid: %s\n", made.Group, made.Test.Index, made.Why)
		}
		found = append(found, shop.findingsOf(made.Warnings)...)
	}
	if invalid > 0 {
		fmt.Printf("\n%d test(s) the validator refuses\n", invalid)
	}

	for _, solution := range shop.Problem.Solutions {
		if only != "" && solution.Name != only {
			continue
		}
		attempt, err := shop.Evaluate(ctx, solution.Name, &Program{Source: solution.Source})
		if err != nil {
			fmt.Fprintln(os.Stderr, "eo-judge:", err)
			return 3
		}
		fmt.Printf("\n%s: %s, %g\n", solution.Name, attempt.Verdict, attempt.Score)
		for _, group := range attempt.Groups {
			fmt.Printf("  testset %-2d %-20s %7.4g of %-7.4g", group.Index, group.Verdict, group.Score, group.Cost)
			fmt.Printf("  %s\n", tally(group))
		}
		for _, group := range attempt.Groups {
			for _, one := range group.Runs {
				found = append(found, shop.findingsOf(one.Warnings)...)
			}
		}
	}

	fmt.Println()
	return report(found, strict)
}

func tally(group *GroupResult) string {
	counted := map[Verdict]int{}
	for _, one := range group.Runs {
		counted[one.Verdict]++
	}
	kinds := make([]string, 0, len(counted))
	for verdict, count := range counted {
		kinds = append(kinds, fmt.Sprintf("%d %s", count, verdict))
	}
	sort.Strings(kinds)
	out := ""
	for at, one := range kinds {
		if at > 0 {
			out += ", "
		}
		out += one
	}
	return out
}

func report(found Findings, strict bool) int {
	seen := map[string]bool{}
	var kept Findings
	for _, one := range found {
		key := one.Code + "|" + one.Where + "|" + one.Message
		if seen[key] {
			continue
		}
		seen[key] = true
		kept = append(kept, one)
	}

	sort.SliceStable(kept, func(i, j int) bool {
		if kept[i].Severity != kept[j].Severity {
			return kept[i].Severity == "warning"
		}
		return kept[i].Code < kept[j].Code
	})

	warnings := 0
	for _, one := range kept {
		fmt.Println(one)
		if one.Severity == "warning" {
			warnings++
		}
	}

	fmt.Printf("\neo-judge: %d warning(s), %d note(s)\n", warnings, len(kept)-warnings)
	if strict && warnings > 0 {
		return 1
	}
	return 0
}
