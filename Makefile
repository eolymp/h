CXX ?= c++
CXXSTD ?= c++17
WARNINGS := -Wall -Wextra -Wshadow -Werror
SOURCES := $(wildcard src/*.h) $(wildcard src/shapes/*.h)
TESTS := tests/all.cpp tests/harness.h $(wildcard tests/*.inc)

.PHONY: all check amalgamate amalgamation-check test coverage e2e standards hostile budget examples codes version mutants sanitize judge pin clean

all: eolymp.h eolymp-shapes.h

eolymp.h: $(SOURCES) tools/amalgamate.py
	python3 tools/amalgamate.py

eolymp-shapes.h: eolymp.h
	@test -f $@ || python3 tools/amalgamate.py

amalgamate:
	python3 tools/amalgamate.py

amalgamation-check:
	python3 tools/amalgamate.py --check

STANDARDS := c++17 c++20 c++23

build/tests-%: eolymp.h eolymp-shapes.h $(TESTS)
	@mkdir -p build
	$(CXX) -std=$* -O1 $(WARNINGS) -DEOLYMP_TESTING -o $@ tests/all.cpp

test: build/tests-$(CXXSTD)
	./build/tests-$(CXXSTD)

coverage: eolymp.h eolymp-shapes.h
	python3 tools/coverage.py

e2e: eolymp.h eolymp-shapes.h
	sh tests/e2e/run.sh

standards: $(STANDARDS:%=build/tests-%)
	@for standard in $(STANDARDS); do echo "standards: $$standard"; ./build/tests-$$standard || exit 1; done

hostile: eolymp.h eolymp-shapes.h
	sh tests/hostile/run.sh

budget: eolymp.h eolymp-shapes.h
	python3 tools/budget.py

examples: eolymp.h eolymp-shapes.h
	python3 tools/examples.py

codes:
	python3 tools/codes.py

version:
	python3 tools/version.py $(BASE)

SANITIZERS := -fsanitize=address,undefined -fno-sanitize-recover=all

SANITIZED := ASAN_OPTIONS=exitcode=86:halt_on_error=1 UBSAN_OPTIONS=exitcode=86:halt_on_error=1

sanitize: eolymp.h eolymp-shapes.h $(TESTS)
	@mkdir -p build
	$(CXX) -std=$(CXXSTD) -O1 -g $(WARNINGS) $(SANITIZERS) -DEOLYMP_TESTING -o build/tests-sanitized tests/all.cpp
	$(SANITIZED) ./build/tests-sanitized
	$(SANITIZED) E2E_BUILD="$(CURDIR)/build/e2e-sanitized" CXX="$(CXX) $(SANITIZERS)" sh tests/e2e/run.sh

mutants: eolymp.h eolymp-shapes.h
	@mkdir -p build
	python3 tools/mutants.py

check: amalgamation-check
	$(MAKE) test coverage standards e2e hostile examples codes
	$(MAKE) budget

judge: eolymp.h eolymp-shapes.h
	cd judge && unformatted=$$(gofmt -l .) && \
		if [ -n "$$unformatted" ]; then echo "judge: gofmt would change $$unformatted" >&2; exit 1; fi && \
		go vet ./... && go test -count=1 ./...

pin:
	cd judge && AGENT_REPO=$(or $(AGENT_REPO),../../agent) go test -count=1 -run TestTheCopiedPointsParser -v ./...

clean:
	rm -rf build
	rm -f *.gcov
