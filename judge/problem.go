package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

type Program struct {
	Source  string   `json:"source"`
	Runtime string   `json:"runtime"`
	Files   []string `json:"files"`
}

type Generator struct {
	Script    string   `json:"script"`
	Arguments []string `json:"arguments"`
}

type Test struct {
	Index           int        `json:"index"`
	Score           float64    `json:"score"`
	Example         bool       `json:"example"`
	Input           string     `json:"input"`
	Answer          string     `json:"answer"`
	Generator       *Generator `json:"generator"`
	AnswerGenerator string     `json:"answerGenerator"`
}

type Testset struct {
	Index          int     `json:"index"`
	ScoringMode    string  `json:"scoringMode"`
	FeedbackPolicy string  `json:"feedbackPolicy"`
	DependencyMode string  `json:"dependencyMode"`
	Dependencies   []int   `json:"dependencies"`
	TimeLimit      int     `json:"timeLimit"`
	MemoryLimit    int64   `json:"memoryLimit"`
	Tests          []*Test `json:"tests"`
}

type Solution struct {
	Name   string `json:"name"`
	Source string `json:"source"`
	Type   string `json:"type"`
	Scores string `json:"scores"`
}

type Problem struct {
	Title               string              `json:"title"`
	Type                string              `json:"type"`
	RunCount            int                 `json:"runCount"`
	TimeLimit           int                 `json:"timeLimit"`
	CPULimit            int                 `json:"cpuLimit"`
	MemoryLimit         int64               `json:"memoryLimit"`
	InstanceLimit       int                 `json:"instanceLimit"`
	InteractorTimeLimit int                 `json:"interactorTimeLimit"`
	Unique              bool                `json:"uniqueAnswer"`
	ExactFormat         bool                `json:"exactFormat"`
	Checker             *Program            `json:"checker"`
	Validator           *Program            `json:"validator"`
	Interactor          *Program            `json:"interactor"`
	Scripts             map[string]*Program `json:"scripts"`
	Solutions           []*Solution         `json:"solutions"`
	Testsets            []*Testset          `json:"testsets"`

	dir string
}

func (p *Problem) Dir() string { return p.dir }

func (p *Problem) Path(name string) string {
	if filepath.IsAbs(name) {
		return name
	}
	return filepath.Join(p.dir, name)
}

func (p *Problem) Interactive() bool {
	return p.Type == "INTERACTIVE" || p.Type == "COMMUNICATION"
}

func (p *Problem) Testset(index int) *Testset {
	for _, one := range p.Testsets {
		if one.Index == index {
			return one
		}
	}
	return nil
}

func (t *Testset) Limit(p *Problem) (int, int64) {
	limit, memory := t.TimeLimit, t.MemoryLimit
	if limit == 0 {
		limit = p.TimeLimit
	}
	if memory == 0 {
		memory = p.MemoryLimit
	}
	return limit, memory
}

func (s *Solution) Expected() (float64, bool) {
	if s.Scores == "" {
		if s.Type == "CORRECT" {
			return 100, true
		}
		return 0, false
	}
	var want float64
	if _, err := fmt.Sscanf(s.Scores, "%g", &want); err != nil {
		return 0, false
	}
	return want, true
}

func LoadProblem(dir string) (*Problem, error) {
	body, err := os.ReadFile(filepath.Join(dir, "problem.json"))
	if err != nil {
		return nil, err
	}

	problem := &Problem{RunCount: 1, dir: dir}
	if err := json.Unmarshal(body, problem); err != nil {
		return nil, fmt.Errorf("problem.json: %w", err)
	}

	if problem.Type == "" {
		problem.Type = "PROGRAM"
	}
	if problem.RunCount < 1 {
		problem.RunCount = 1
	}
	for _, testset := range problem.Testsets {
		if testset.ScoringMode == "" {
			testset.ScoringMode = "EACH"
		}
		if testset.FeedbackPolicy == "" {
			testset.FeedbackPolicy = "COMPLETE"
		}
		if testset.DependencyMode == "" {
			testset.DependencyMode = "FULLY_ACCEPTED"
		}
	}
	return problem, nil
}
