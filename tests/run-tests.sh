#!/bin/sh
cd "$(dirname "$0")/.." || exit 1
make clean
make CFLAGS='-O3 -fPIC' LDFLAGS='-shared' newsraft
[ -e newsraft ] || exit 1
mv newsraft tests/libnewsraft.so
cd tests
tests_count=0
okays_count=0
fails_count=0
for test_source in *.c
do
	tests_count="$((tests_count + 1))"
	if cc "$test_source" -I../src -L. -l:libnewsraft.so; then
		if env LD_LIBRARY_PATH=. ./a.out; then
			echo "[OKAY] $test_source"
			okays_count="$((okays_count + 1))"
			continue
		fi
	fi
	echo "[FAIL] $test_source"
	fails_count="$((fails_count + 1))"
done
echo "$okays_count/$tests_count TESTS PASSED"
