# Makefile for SGFC
# Copyright (C) 1996-2026 by Arno Hollosi
# (see 'COPYING' for more copyright information)

sgfc:
	$(MAKE) -C src/ sgfc

tests:
	$(MAKE) -C tests/ tests

clean:
	$(MAKE) -C src/ clean
	$(MAKE) -C tests/ clean
	rm -rf coverage-html coverage.info

clean-test-files:
	rm -f test-files/*.txt test-files/*-result.sgf

test-files: src/sgfc
	# "|| true" is necessary, because sgfc returns status code 5 or 10, as the files contain errors
	src/sgfc -c test-files/test.sgf test-files/test-result.sgf 2>test-files/test-output.txt || true
	src/sgfc -roun -c test-files/test.sgf test-files/test-roun-result.sgf 2>test-files/test-roun-output.txt || true
	src/sgfc -rc test-files/strict.sgf test-files/strict-result.sgf 2>test-files/strict-output.txt || true
	src/sgfc -v test-files/reorder.sgf test-files/reorder-result.sgf 2>test-files/reorder-output.txt || true
	src/sgfc -vz test-files/reorder.sgf test-files/reorder-z-result.sgf 2>test-files/reorder-z-output.txt || true
	src/sgfc -ct test-files/escaping.sgf test-files/escaping-result.sgf 2>test-files/escaping-output.txt || true
	src/sgfc -cE2 test-files/mixed-encoding.sgf test-files/mixed-encoding-result.sgf 2>test-files/mixed-encoding-output.txt || true

coverage:
	$(MAKE) -C src sgfc OPTFLAGS='-O0' BUILD_CFLAGS='--coverage -g' LDFLAGS='--coverage'
	$(MAKE) -C tests tests OPTFLAGS='-O0' BUILD_CFLAGS='--coverage -g' LDFLAGS='--coverage'
	lcov --capture --directory src --output-file coverage.info
	genhtml coverage.info --output-directory coverage-html

SANITIZERS := -fsanitize=address,undefined,pointer-compare,pointer-subtract,bounds,object-size,return,unreachable
SANITIZER_OPTIONS := ASAN_OPTIONS=detect_invalid_pointer_pairs=2 \
                     UBSAN_OPTIONS=print_stacktrace=1

sanitize:
	$(MAKE) -C src sgfc \
		OPTFLAGS='-O0' BUILD_CFLAGS='-g -fno-omit-frame-pointer $(SANITIZERS)' LDFLAGS=$(SANITIZERS)
	$(MAKE) -C tests tests OPTFLAGS='-O0' \
		BUILD_CFLAGS='-g -fno-omit-frame-pointer $(SANITIZERS)' LDFLAGS=$(SANITIZERS) \
		RUN_PREFIX='$(SANITIZER_OPTIONS)'

all: clean sgfc tests

.PHONY: sgfc tests test-files clean clean-test-files coverage sanitize
