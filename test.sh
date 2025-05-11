#!/bin/sh

python3 do_test.py --output-file=test.c --exclude=examples/new_lang/safe_cast.own,/home/jonathan/src/compile-lang-2024/examples/new_lang/tuple.own,/home/jonathan/src/compile-lang-2024/tests/expected_failure_examples/tuple_invalid_subtype.own,/home/jonathan/src/compile-lang-2024/tests/expected_failure_examples/tuple_mismatched_counts.own
