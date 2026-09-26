package main

import (
	"errors"
	"fmt"
	"io"
	"os"
)

func main() {
	file, err := os.Open(os.Args[1])
	if err != nil {
		panic(err)
	}
	defer file.Close()
	token := ""
	score := float32(0)
	var failure error = errors.New("no score in the checker log")
	for {
		_, one := fmt.Fscanf(file, "%s", &token)
		if one == io.EOF {
			break
		}
		if one != nil {
			failure = one
			break
		}
		if token != "points" {
			continue
		}
		if _, one = fmt.Fscanf(file, "%f", &score); one != nil && one != io.EOF {
			failure = one
			break
		}
		failure = nil
		break
	}
	if failure != nil {
		fmt.Println("FAIL", failure)
		os.Exit(1)
	}
	fmt.Printf("%v\n", score)
}
