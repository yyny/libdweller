# Unwinding methods

`libwander` will try several method for obtaining unwinding information:

 * `WANDER_UNWIND_METHOD_LIBGCC`
 * `WANDER_UNWIND_METHOD_LIBUNWIND`
 * `WANDER_UNWIND_METHOD_LIBBACKTRACE`
 * `WANDER_UNWIND_METHOD_GNULIBC`
 * `WANDER_UNWIND_METHOD_NAIVE`

If one of the above methods has not been defined in `libwander/config.h`,
support for that method will not be compiled in to the library.

`libwander` will try these methods in the order they are declared.
Next follows an explanation for these methods and their order.

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
