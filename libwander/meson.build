cc = meson.get_compiler('c')

conf = configuration_data()
conf.set( 'WANDER_CONFIG_MAX_STACK_DEPTH',              256   )
conf.set( 'WANDER_CONFIG_MAX_SOURCE_LOCATIONS',         256   )
conf.set( 'WANDER_CONFIG_MAX_SHARED_STRING_SIZE',       4096  ) # Maximum combined size of (directory_name + file_name + function_name)
conf.set( 'WANDER_CONFIG_UNWIND_METHOD_LIBGCC',         1     )
conf.set( 'WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND',      0     ) # FIXME: Detect
conf.set( 'WANDER_CONFIG_UNWIND_METHOD_LIBBACKTRACE',   0     ) # TODO
conf.set( 'WANDER_CONFIG_UNWIND_METHOD_GNULIBC',        1     ) # FIXME: Detect
if host_machine.system() == 'windows'
    conf.set( 'WANDER_CONFIG_UNWIND_METHOD_DBGHELP',    1     )
endif
conf.set( 'WANDER_CONFIG_UNWIND_METHOD_NAIVE',          0     )
conf.set( 'WANDER_CONFIG_HAVE_PTHREAD_GETTHREADID_NP',  false ) # Define to 1 if you have the `pthread_getthreadid_np' function.
conf.set( 'WANDER_CONFIG_HAVE_PTHREAD_NP_H',            false ) # Define to 1 if you have the <pthread_np.h> header file.
configure_file(configuration : conf, output : 'libwander_config.h')

libwander_inc = include_directories('.', 'include')
libwander_src = files('src/wander.c', 'src/wander_platform.c', 'src/wander_resolver.c', 'src/wander_printer.c')

libdl = cc.find_library('dl', required : false)
# NOTE about -D_GNU_SOURCE:
# The unwinder requires GNU extensions to reliably skip stack frames inside of signal handlers.
# This features is placed behind #ifdef's with a runtime fallback, so no portability is lost.
# See `wander_handle_sigaction` for details.
# NOTE: On unix, `wander_resolver.c` uses `dl_iterate_phdr`, which is a GNU extension, and not supported on all systems.
# `EnumProcessModules` is used on windows.
libwander = library('wander', libwander_src, include_directories : libwander_inc, dependencies : [ libdweller_dep, libdl ], c_args : ['-D_GNU_SOURCE'])
libwander_dep = declare_dependency(include_directories : libwander_inc, link_with : libwander)
