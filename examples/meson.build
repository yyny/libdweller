iterate_phdr = executable('iterate_phdr', files('00_iterate_phdr.c'), dependencies : libdweller_dep)
stacktrace = executable('stacktrace', files('01_stacktrace.c'), dependencies : libdweller_dep, c_args : ['-g']) # Make an exception for debug symbols in release mode since stacktrace doesn't really do anything without them

examples = [iterate_phdr, stacktrace]
