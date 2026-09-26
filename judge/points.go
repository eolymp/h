package main

import (
	"errors"
	"fmt"
	"io"
)

func readPoints(r io.Reader) (float32, error) {
	token := ""
	score := float32(0)

	for {
		_, err := fmt.Fscanf(r, "%s", &token)
		if err == io.EOF {
			break
		}

		if err != nil {
			return 0, err
		}

		if token != "points" {
			continue
		}

		_, err = fmt.Fscanf(r, "%f", &score)
		if err == io.EOF {
			break
		}

		if err != nil {
			return 0, err
		}

		return score, nil
	}

	return 0, errors.New("no score in the checker log")
}
