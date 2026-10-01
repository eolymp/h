package main

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"sync"
)

const (
	transcriptLines = 1000
	transcriptWidth = 200
	transcriptBytes = 4 * transcriptWidth
)

type dialogue struct {
	mu      sync.Mutex
	lines   []string
	omitted int
	waiting sync.WaitGroup
}

func (d *dialogue) say(who string, line []byte) {
	d.mu.Lock()
	defer d.mu.Unlock()
	if len(d.lines) >= transcriptLines {
		d.omitted++
		return
	}
	text := string(bytes.TrimSuffix(line, []byte("\r")))
	if runes := []rune(text); len(runes) > transcriptWidth {
		text = string(runes[:transcriptWidth]) + "…"
	}
	d.lines = append(d.lines, who+text)
}

func (d *dialogue) relay(who string, from, to *os.File) {
	d.waiting.Add(1)
	go func() {
		defer d.waiting.Done()
		defer from.Close()
		defer to.Close()
		var partial []byte
		chunk := make([]byte, 64<<10)
		for {
			got, err := from.Read(chunk)
			if got > 0 {
				if _, failed := to.Write(chunk[:got]); failed != nil {
					return
				}
				for rest := chunk[:got]; len(rest) > 0; {
					end := bytes.IndexByte(rest, '\n')
					piece := rest
					if end >= 0 {
						piece = rest[:end]
					}
					room := transcriptBytes - len(partial)
					partial = append(partial, piece[:min(room, len(piece))]...)
					if end < 0 {
						break
					}
					d.say(who, partial)
					partial, rest = partial[:0], rest[end+1:]
				}
			}
			if err != nil {
				if err == io.EOF && len(partial) > 0 {
					d.say(who, partial)
				}
				return
			}
		}
	}()
}

func (d *dialogue) finished() []string {
	d.waiting.Wait()
	if d.omitted > 0 {
		return append(d.lines, fmt.Sprintf("… %d more lines", d.omitted))
	}
	return d.lines
}
