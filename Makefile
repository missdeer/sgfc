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
	src/sgfc -c test-files/test.sgf test-files/test-result.sgf >test-files/test-output.txt || true
	src/sgfc -roun -c test-files/test.sgf test-files/test-roun-result.sgf >test-files/test-roun-output.txt || true
	src/sgfc -rc test-files/strict.sgf test-files/strict-result.sgf >test-files/strict-output.txt || true
	src/sgfc -v test-files/reorder.sgf test-files/reorder-result.sgf >test-files/reorder-output.txt || true
	src/sgfc -vz test-files/reorder.sgf test-files/reorder-z-result.sgf >test-files/reorder-z-output.txt || true
	src/sgfc -ct test-files/escaping.sgf test-files/escaping-result.sgf >test-files/escaping-output.txt || true
	src/sgfc -cE2 test-files/mixed-encoding.sgf test-files/mixed-encoding-result.sgf >test-files/mixed-encoding-output.txt || true

coverage:
	$(MAKE) -C src sgfc OPTFLAGS='-O0 -g' COVERAGEFLAGS='--coverage'
	$(MAKE) -C tests tests OPTFLAGS='-O0 -g' COVERAGEFLAGS='--coverage'
	lcov --capture --directory src --output-file coverage.info
	genhtml coverage.info --output-directory coverage-html

all: clean sgfc tests

.PHONY: sgfc tests test-files clean clean-test-files coverage
