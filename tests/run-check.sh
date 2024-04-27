#!/bin/sh
cd "$(dirname "$0")" || exit 1
rm -rf makefile src
cp -r ../makefile ../src .
make clean
make CFLAGS='-O3 -fPIC -DTEST' libnewsraft.so
[ -e libnewsraft.so ] || exit 1
tests_count=0
okays_count=0
fails_count=0
echo
for test_file in *.c
do
	tests_count="$((tests_count + 1))"
	rm -rf test-database*
	if ${CC:-cc} -Isrc -DTEST -o ./a.out "$test_file" -L. -lnewsraft && env LD_LIBRARY_PATH=. ./a.out; then
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
[ "$okays_count" = "$tests_count" ] || exit 1
