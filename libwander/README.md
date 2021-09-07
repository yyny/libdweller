`libwander` is a simple stack unwinding library capable of producing AS-safe backtraces.
`libwander` can currently only resolve debug info using `libdweller`,
although support for `libdwarf`, `libdw` and possibly others (like microsoft's "Symbol Server"s) might at some point be added aswell.

For an example, see `./examples/01_stacktrace.c`.
An example output produced by `libwander`:

```
$ ./build/examples/stacktrace
Stack trace (most recent call last):
#14 0x0000563f037ff09a in [_start + 0x2a] at ../sysdeps/x86_64/start.S:120
#13 0x00007fdc24098e0a in __libc_start_main() at ../csu/libc-start.c:314
#12 0x0000563f037ff1c3 in main() at ../examples/01_stacktrace.c:24
#11 0x0000563f037ff192 in recurse() at ../examples/01_stacktrace.c:16
#10 0x0000563f037ff192 in recurse() at ../examples/01_stacktrace.c:16
#9  0x0000563f037ff192 in recurse() at ../examples/01_stacktrace.c:16
#8  0x0000563f037ff192 in recurse() at ../examples/01_stacktrace.c:16
#7  0x0000563f037ff192 in recurse() at ../examples/01_stacktrace.c:16
#6  0x0000563f037ff199 in recurse() at ../examples/01_stacktrace.c:17
#5  0x0000563f037ff166 in inlined_function() at ../examples/01_stacktrace.c:11
#4  0x00007fdc240ae000 in [__restore_rt + 0x0] from /usr/lib/libc.so.6
#3  0x00007fdc242554cd in wander_sigaction_handler() at ../libwander/src/wander.c:17
#2  0x00007fdc242555c2 in wander_handle_sigaction() at ../libwander/src/wander.c:72
#1  0x00007fdc24255b4f in wander_backtrace_safe() at ../libwander/src/wander.c:213
#0  0x00007fdc242564a1 in wander_platform_backtrace() at ../libwander/src/wander_platform.c:269
Segmentation fault (Address not mapped to object [0xdeadbeef])
Segmentation fault
```

# Unwinding methods

`libwander` will try several method for obtaining unwinding information:

 * `WANDER_UNWIND_METHOD_DBGHELP` (win32)
 * `WANDER_UNWIND_METHOD_LIBGCC`
 * `WANDER_UNWIND_METHOD_LIBUNWIND`
 * `WANDER_UNWIND_METHOD_LIBBACKTRACE`
 * `WANDER_UNWIND_METHOD_GNULIBC`
 * `WANDER_UNWIND_METHOD_NAIVE`

If one of the above methods has not been defined in `libwander/config.h`,
support for that method will not be compiled in to the library.

`libwander` will try these methods in the order they are declared.
Next follows an explanation for these methods and their order.

## `WANDER_UNWIND_METHOD_DBGHELP` (win32)

On `win32`, this method uses the `CaptureStackBackTrace` function declared in `dbghelp.h`,
included with `DbgHelp.lib`.
The `win32` platform has no concept of AS-safety,
although the `WANDER_UNWIND_METHOD_DBGHELP` method can generally be considered AS-safe.
Because this is the recommended API for capturing backtraces on `win32`,
this method is always tried first when available.

## `WANDER_UNWIND_METHOD_LIBGCC`

The `WANDER_UNWIND_METHOD_LIBGCC` method uses the `_Unwind_Backtrace` and `_Unwind_GetIP`
functions provided by `libgcc.so`, which is already installed on most unix systems.
The `WANDER_UNWIND_METHOD_LIBGCC` is a lightweight method without much overhead.
It is the same method typically used to implement C++ exceptions.
For all of these reasons, this method is tried early.

## `WANDER_UNWIND_METHOD_LIBUNWIND`

The `WANDER_UNWIND_METHOD_LIBUNWIND` method uses the [`libunwind`](https://www.nongnu.org/libunwind/)
library to obtain reliable, high quality unwinding information.
While the `libunwind` library is fast and reliable, it is quite bloated for a simple unwinding library
(The dynamic library files alone take more space than the entirity of `libgcc`), and for that
reason alone it is probably not installed on most systems.
The maintainers of `libunwind` also seem to have a backwards idea of AS-safety.
Still, the library is well written and used by several high profile projects.
If `libunwind` is installed, it will be tried with high priority.

## `WANDER_UNWIND_METHOD_LIBBACKTRACE`

This method is not yet implemented.

## `WANDER_UNWIND_METHOD_GNULIBC`

The `WANDER_UNWIND_METHOD_GNULIBC` method uses the `backtrace()` function declared in `execinfo.h`,
included with `glibc`.
As with `WANDER_UNWIND_METHOD_LIBUNWIND`, unwinding using `WANDER_UNWIND_METHOD_GNULIBC` can't be
trusted to be AS-safe, although measures are taking to improve it's safety.
While this method is simple and reliable, `glibc` uses `libgcc` internally anyway.
This method is also not AS-safe. For these reasons, it is typically only tried as a last resort.

## `WANDER_UNWIND_METHOD_NAIVE`

This method will try a very rudimentary unwinding method that only works on x86.
It is disabled by default, and is always tried last.
