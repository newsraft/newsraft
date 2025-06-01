#!/bin/sh

set -e

log_file='newsraft-test-log'
green="$(printf '\x1b[32m')"
red="$(printf '\x1b[31m')"
reset="$(printf '\x1b[0m')"

cd "$(dirname "$0")/.."

make CFLAGS='-O3 -fPIC' libnewsraft.so

test -e libnewsraft.so

rm -f "$log_file"

echo
for test_file in tests/*.c; do
	status=0
	rm -rf newsraft-test-database*
	make TEST_FILE="$test_file" test-program 1>>"$log_file" 2>&1
	env LD_LIBRARY_PATH=. ./newsraft-test 2>&1 || status="$?"
	echo "TEST_STATUS:$status" >> "$log_file"
	echo "[$([ "$status" = 0 ] && echo "${green}OKAY" || echo "${red}FAIL")${reset}] $test_file"
done | tee -a "$log_file"

okays_count="$(grep -c TEST_STATUS:0 "$log_file")"
tests_count="$(grep -c TEST_STATUS:  "$log_file")"

echo
echo "$okays_count/$tests_count TESTS PASSED"
echo
if [ "$okays_count" != "$tests_count" ]; then
	echo "Look for details in the ./$log_file"
	echo
	exit 1
fi
