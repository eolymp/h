CXX ?= c++
CXXSTD ?= c++17
WARNINGS := -Wall -Wextra -Wshadow -Werror
SOURCES := $(wildcard src/*.h) $(wildcard src/shapes/*.h)
TESTS := tests/all.cpp tests/harness.h $(wildcard tests/*.inc)

.PHONY: all check amalgamate amalgamation-check test coverage e2e standards hostile budget examples codes mutants judge pin clean

all: eolymp.h eolymp-shapes.h

eolymp.h eolymp-shapes.h: $(SOURCES) tools/amalgamate.py
	python3 tools/amalgamate.py

amalgamate:
	python3 tools/amalgamate.py

amalgamation-check:
	python3 tools/amalgamate.py --check

build/tests: eolymp.h eolymp-shapes.h $(TESTS)
	@mkdir -p build
	$(CXX) -std=$(CXXSTD) -O1 $(WARNINGS) -DEOLYMP_TESTING -o $@ tests/all.cpp

test: build/tests
	./build/tests

coverage: eolymp.h eolymp-shapes.h
	python3 tools/coverage.py

e2e: eolymp.h eolymp-shapes.h
	sh tests/e2e/run.sh

standards: eolymp.h eolymp-shapes.h
	@mkdir -p build
	@for standard in c++17 c++20 c++23; do \
		echo "standards: $$standard"; \
		$(CXX) -std=$$standard -O1 $(WARNINGS) -DEOLYMP_TESTING -o build/tests-$$standard tests/all.cpp || exit 1; \
		./build/tests-$$standard || exit 1; \
	done

hostile: eolymp.h eolymp-shapes.h
	sh tests/hostile/run.sh

budget: eolymp.h eolymp-shapes.h
	python3 tools/budget.py

examples: eolymp.h eolymp-shapes.h
	python3 tools/examples.py

codes:
	python3 tools/codes.py

mutants: eolymp.h eolymp-shapes.h
	@mkdir -p build
	python3 tools/mutants.py

check: amalgamation-check test coverage standards e2e hostile examples codes budget

judge: eolymp.h eolymp-shapes.h
	cd judge && gofmt -l . | tee /dev/stderr | (! read) && go vet ./... && go test -count=1 ./...

pin:
	cd judge && AGENT_REPO=$(or $(AGENT_REPO),../../agent) go test -count=1 -run TestTheCopiedPointsParser -v ./...

clean:
	rm -rf build
	rm -f *.gcov
