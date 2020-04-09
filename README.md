# libdweller

`libdweller` is a library for parsing DWARF debug data.

I hacked it together over the course of about 3 days, so the code is still
very hacky and experimental, but hopefully with time I can make it more
stable and production-ready.

## Building

```
$ meson . build
$ ninja -C build
$ ./build/examples/stacktrace
```

## Testing

```
(cd build; meson test --no-rebuild)
```

## Performance

This comparison is in no way representative, since dweller is still very
feature-incomplete, but considering dwarfdump can already successfully provide
most of the same information that existing tools can I was quite impressed by
the performance thus far.

![Benchmark plot](./docs/bench.png)

<details>
<summary>Results</summary>

### libdweller (debug, initial commit)

```
$ libc="/usr/lib/debug/lib/x86_64-linux-gnu/libc-2.27.so"
$ time ./build/dwarfdump/dwarfdump $libc > /dev/null

real    0m2.641s
user    0m2.565s
sys     0m0.060s
```

### libdweller (release, latest version)

```
$ libc="/usr/lib/debug/lib/x86_64-linux-gnu/libc-2.27.so"
$ time ./build/dwarfdump/dwarfdump $libc > /dev/null

real    0m0.316s
user    0m0.295s
sys     0m0.004s
```

### libdwarf (stripped)

```
$ time dwarfdump $libc > /dev/null

real    0m4.132s
user    0m4.080s
sys     0m0.048s
```

### GNU binutils (stripped)

```
$ time readelf --debug $libc > /dev/null

real    0m3.845s
user    0m3.808s
sys     0m0.032s
```

### Rust Gimli (debug)

```
$ time ./target/debug/examples/dwarfdump $libc > /dev/null

real    0m12.111s
user    0m26.364s
sys     0m0.032s
```

### Rust Gimli (stripped release)

```
$ time ./target/release/examples/dwarfdump $libc > /dev/null

real    0m0.743s
user    0m1.478s
sys     0m0.008s
```

</details>
