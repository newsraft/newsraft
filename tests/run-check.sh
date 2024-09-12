#!/bin/sh

set -e

log_file='newsraft-test-log'

cd "$(dirname "$0")/.."

make clean
make CFLAGS='-O3 -fPIC' libnewsraft.so

test -e libnewsraft.so

echo
tests_count=0
okays_count=0
for test_file in tests/*.c; do
	status='FAIL'
	tests_count="$((tests_count + 1))"
	rm -rf newsraft-test-database*
	make TEST_FILE="$test_file" test-program 1>>"$log_file" 2>&1
	if env LD_LIBRARY_PATH=. ./newsraft-test 2>&1 | tee -a "$log_file"; then
		status='OKAY'
		okays_count="$((okays_count + 1))"
	fi
	echo "[$status] $test_file" | tee -a "$log_file"
done

echo
echo "$okays_count/$tests_count TESTS PASSED"
echo
if [ "$okays_count" != "$tests_count" ]; then
	echo "Look for details in the ./$log_file"
	echo
	exit 1
fi
