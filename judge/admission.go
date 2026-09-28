package main

type Outcome int

const (
	Admitted Outcome = iota
	Blocked
	Rejected
)

type Controller interface {
	Admit(run *Planned) Outcome
	Notify(run *Planned, result *RunResult)
}

type Planned struct {
	Group int
	Test  *Test
}

type showstopper struct {
	group    int
	rejected bool
}

func (a *showstopper) Admit(run *Planned) Outcome {
	if run.Group != a.group || !a.rejected {
		return Admitted
	}
	return Rejected
}

func (a *showstopper) Notify(run *Planned, result *RunResult) {
	if run.Group != a.group {
		return
	}
	switch result.Verdict {
	case Accepted:
		return
	case WrongAnswer, Partial:
		if result.Score == 0 {
			a.rejected = true
		}
	default:
		a.rejected = true
	}
}

type dependency struct {
	group    int
	blockers map[string]bool
	mode     string
	rejected bool
}

func (a *dependency) Admit(run *Planned) Outcome {
	if run.Group != a.group || len(a.blockers) == 0 {
		return Admitted
	}
	if a.rejected {
		return Rejected
	}
	return Blocked
}

func (a *dependency) Notify(run *Planned, result *RunResult) {
	if !a.blockers[reference(run)] {
		return
	}
	if a.mode == "FIRST_POINT" {
		if result.Verdict == Accepted || ((result.Verdict == WrongAnswer || result.Verdict == Partial) && result.Score > 0) {
			a.blockers = nil
		}
		return
	}
	if result.Verdict == Accepted {
		delete(a.blockers, reference(run))
		return
	}
	a.rejected = true
}

type chain struct {
	controllers []Controller
}

func (a *chain) Admit(run *Planned) Outcome {
	for _, one := range a.controllers {
		if outcome := one.Admit(run); outcome != Admitted {
			return outcome
		}
	}
	return Admitted
}

func (a *chain) Notify(run *Planned, result *RunResult) {
	for _, one := range a.controllers {
		one.Notify(run, result)
	}
}

func admissionFor(problem *Problem) *chain {
	built := &chain{}
	for _, testset := range problem.Testsets {
		if testset.FeedbackPolicy == "ICPC" || testset.FeedbackPolicy == "ICPC_EXPANDED" {
			built.controllers = append(built.controllers, &showstopper{group: testset.Index})
		}
		if len(testset.Dependencies) == 0 {
			continue
		}
		blockers := map[string]bool{}
		for _, on := range testset.Dependencies {
			source := problem.Testset(on)
			if source == nil {
				continue
			}
			for _, test := range source.Tests {
				blockers[reference(&Planned{Group: source.Index, Test: test})] = true
			}
		}
		built.controllers = append(built.controllers,
			&dependency{group: testset.Index, blockers: blockers, mode: testset.DependencyMode})
	}
	return built
}
