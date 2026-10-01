package main

import (
	"io"
	"os"
	"path/filepath"
	"regexp"
	"strings"
	"testing"
)

func TestTranscriptPrintsTheDialogueOfEveryRun(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	code, out, errs := invoke("run", "../tests/live/guess", "--solution", "binary", "--transcript")
	if code != 0 {
		t.Fatalf("run exited %d, printed %q, said %q", code, out, errs)
	}
	run := printedRun(out, "    1:2 ACCEPTED", "    1:3 ")
	for _, want := range []string{"\n      interactor: 1000\n      solution:   ? 500\n      interactor: >\n",
		"\n      solution:   ! 1\n"} {
		if !strings.Contains(run, want) {
			t.Errorf("the transcript of 1:2 has no %q:\n%s", want, run)
		}
	}
	if strings.Contains(run, "phase") {
		t.Errorf("a problem of one run names a phase:\n%s", run)
	}

	code, out, errs = invoke("run", "../tests/live/guess", "--solution", "binary", "--transcript", "--json")
	if code != 0 || !strings.Contains(out, `"transcript": [`) || !strings.Contains(out, `"solution:   ? `) {
		t.Errorf("--json exited %d, printed %q, said %q", code, out, errs)
	}
	code, out, _ = invoke("run", "../tests/live/guess", "--solution", "binary", "--json")
	if code != 0 || strings.Contains(out, `"transcript"`) {
		t.Errorf("a run without --transcript carries one: %q", out)
	}
}

func TestTranscriptNamesEveryPhase(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	dir := filepath.Join(t.TempDir(), "phases")
	if code, out, errs := invoke("init", dir, "--type", "phases"); code != 0 {
		t.Fatalf("init exited %d, printed %q, said %q", code, out, errs)
	}
	code, out, errs := invoke("run", dir, "--solution", "binary", "--transcript")
	run := printedRun(out, "    1:1 ACCEPTED", "    1:2 ")
	if code != 0 || !strings.Contains(run, "\n      phase 1\n      interactor: ") ||
		!strings.Contains(run, "\n      phase 2\n      interactor: ") {
		t.Errorf("run exited %d, said %q, and 1:1 reads:\n%s", code, errs, run)
	}
}

func TestTranscriptIsForAnInteractiveProblem(t *testing.T) {
	t.Parallel()
	for _, args := range [][]string{
		{"run", "../tests/live/degrees", "--transcript"},
		{"check", "../tests/live/guess", "--transcript"},
	} {
		code, out, errs := invoke(args...)
		if code != 2 || out != "" || !strings.Contains(errs, "--transcript prints the dialogue of an interactive "+
			"problem's runs, so it applies to run on an INTERACTIVE problem") {
			t.Errorf("%q exited %d, printed %q, said %q", args, code, out, errs)
		}
	}
}

func printedRun(text, from, to string) string {
	start := strings.Index(text, from)
	if start < 0 {
		return ""
	}
	text = text[start:]
	if stop := strings.Index(text, to); stop >= 0 {
		text = text[:stop]
	}
	return text
}

func verdictLines(out string) string {
	var kept []string
	for _, line := range strings.Split(out, "\n") {
		if strings.HasPrefix(line, "      ") {
			continue
		}
		kept = append(kept, regexp.MustCompile(` [0-9]+ms `).ReplaceAllString(line, " "))
	}
	return strings.Join(kept, "\n")
}

func TestTranscriptKeepsTheVerdictOfASideThatWritesToAPeerThatLeft(t *testing.T) {
	t.Parallel()
	needsACompiler(t)

	guess, err := filepath.Abs("../tests/live/guess")
	if err != nil {
		t.Fatal(err)
	}
	late := writeProblem(t, map[string]string{
		"problem.json": `{"type": "INTERACTIVE", "timeLimit": 2000,
			"checker": {"source": "` + guess + `/checker.cpp"}, "interactor": {"source": "` + guess + `/interactor.cpp"},
			"solutions": [{"name": "late", "source": "late.cpp"}],
			"testsets": [{"index": 1, "tests": [{"index": 1, "score": 100, "input": "` + guess + `/tests/02.txt"}]}]}`,
		"late.cpp": `#include <cstdio>
#include <unistd.h>
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    int low = 1, high = n;
    while (low < high) {
        int const middle = low + (high - low) / 2;
        std::printf("? %d\n", middle);
        std::fflush(stdout);
        char reply[8];
        if (std::scanf("%7s", reply) != 1) return 1;
        if (reply[0] == '=') { low = high = middle; break; }
        if (reply[0] == '<') low = middle + 1; else high = middle - 1;
    }
    std::printf("! %d\n", low);
    std::fflush(stdout);
    usleep(1000000);
    std::printf("thanks\n");
    std::fflush(stdout);
}
`})
	chatty := writeProblem(t, map[string]string{
		"problem.json": `{"type": "INTERACTIVE", "timeLimit": 2000,
			"checker": {"source": "checker.cpp"}, "interactor": {"source": "interactor.cpp"},
			"solutions": [{"name": "brief", "source": "brief.cpp"}],
			"testsets": [{"index": 1, "tests": [{"index": 1, "score": 100, "input": "in.txt"}]}]}`,
		"in.txt":      "1\n",
		"checker.cpp": "int main() { return 0; }\n",
		"interactor.cpp": `#include <cstdio>
#include <unistd.h>
int main(int, char** argv) {
    std::printf("1\n");
    std::fflush(stdout);
    char line[64];
    if (!std::fgets(line, sizeof line, stdin)) return 1;
    usleep(1000000);
    std::printf("bye\n");
    std::fflush(stdout);
    std::FILE* out = std::fopen(argv[2], "w");
    std::fputs("ok\n", out);
    std::fclose(out);
}
`,
		"brief.cpp": `#include <cstdio>
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::printf("! %d\n", n);
    std::fflush(stdout);
}
`})
	for dir, want := range map[string]string{late: "1:1 RUNTIME_ERROR", chatty: "1:1 FAILURE"} {
		code, plain, errs := invoke("run", "-v", dir)
		if code != 0 || !strings.Contains(plain, want) {
			t.Fatalf("run -v exited %d, printed %q, said %q; want %s", code, plain, errs, want)
		}
		code, recorded, errs := invoke("run", "--transcript", dir)
		if code != 0 || verdictLines(recorded) != verdictLines(plain) {
			t.Errorf("--transcript exited %d, said %q, and printed\n%s\nwhere -v printed\n%s", code, errs,
				recorded, plain)
		}
	}
}

func TestATranscriptKeepsAThousandLinesOfTwoHundredCharacters(t *testing.T) {
	t.Parallel()
	record := &dialogue{}
	for at := 0; at < transcriptLines+5; at++ {
		record.say("solution:   ", []byte("? 1\r"))
	}
	record.say("solution:   ", []byte(strings.Repeat("я", 300)))
	lines := record.finished()
	if len(lines) != transcriptLines+1 || lines[0] != "solution:   ? 1" || lines[len(lines)-1] != "… 6 more lines" {
		t.Errorf("kept %d lines, the first %q and the last %q", len(lines), lines[0], lines[len(lines)-1])
	}
	long := &dialogue{}
	long.say("interactor: ", []byte(strings.Repeat("я", 300)))
	if got := long.finished()[0]; got != "interactor: "+strings.Repeat("я", transcriptWidth)+"…" {
		t.Errorf("a long line was cut to %q", got)
	}
}

func TestARelayKeepsTheStartOfALineThatNeverEnds(t *testing.T) {
	t.Parallel()
	from, into, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}
	out, to, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}
	record := &dialogue{}
	record.relay("solution:   ", from, to)
	go func() {
		into.Write([]byte(strings.Repeat("x", 3<<20) + "\nlast"))
		into.Close()
	}()
	copied, err := io.ReadAll(out)
	if err != nil || len(copied) != 3<<20+5 {
		t.Errorf("the relay passed on %d bytes (%v), not %d", len(copied), err, 3<<20+5)
	}
	lines := record.finished()
	if len(lines) != 2 || lines[0] != "solution:   "+strings.Repeat("x", transcriptWidth)+"…" ||
		lines[1] != "solution:   last" {
		t.Errorf("the relay recorded %d lines: %.80q", len(lines), lines)
	}
}
