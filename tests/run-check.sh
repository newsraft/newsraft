#!/bin/sh
set -e
cd "$(dirname "$0")/.."
make clean
make CFLAGS='-O3 -fPIC' libnewsraft.so
test -e libnewsraft.so
tests_count=0
okays_count=0
fails_count=0
echo
for test_file in tests/*.c; do
	tests_count="$((tests_count + 1))"
	rm -rf newsraft-test-database*
	make TEST_FILE="$test_file" test-program 1>>newsraft-test-log 2>&1
	if env LD_LIBRARY_PATH=. ./newsraft-test; then
		echo "[OKAY] $test_file"
		okays_count="$((okays_count + 1))"
	else
		echo "[FAIL] $test_file"
		fails_count="$((fails_count + 1))"
	fi
done
echo
echo "$okays_count/$tests_count TESTS PASSED"
echo
if [ "$okays_count" != "$tests_count" ]; then
	echo "Look for details in the ./newsraft-test-log"
	echo
	exit 1
fi
