#!/bin/bash
libc="/usr/lib/debug/lib/x86_64-linux-gnu/libc-2.31.so"
bench() {
	echo --- $1 ---
	time $2 "$libc" > /dev/null
	time $2 "$libc" > /dev/null
	time $2 "$libc" > /dev/null
	time $2 "$libc" > /dev/null
	time $2 "$libc" > /dev/null
}
bench "libdweller" ./build-clang/dwarfdump/dwarfdump
bench "libdwarf" dwarfdump
bench "GNU binutils" "readelf --debug"
bench "gimli" ../gimli/target/release/examples/dwarfdump
