cc = meson.get_compiler('c')

stacktrace = executable('stacktrace', files('01_stacktrace.c'), dependencies : libwander_dep, c_args : ['-O0', '-g']) # Make an exception for debug symbols in release mode since stacktrace doesn't really do anything without them

examples = [stacktrace]

if cc.has_function('dl_iterate_phdr')
    iterate_phdr = executable('iterate_phdr', files('00_iterate_phdr.c'), dependencies : libdweller_dep)
    examples += [iterate_phdr]
endif
