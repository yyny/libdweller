cc = meson.get_compiler('c')

libwander_src = files('src/wander.c', 'src/wander_platform.c', 'src/wander_resolver.c', 'src/wander_printer.c')
libwander_inc = include_directories('include')

libdl = cc.find_library('dl')
# NOTE about -D_GNU_SOURCE:
# The unwinder requires GNU extensions to reliably skip stack frames inside of signal handlers.
# This features is placed behind #ifdef's with a runtime fallback, so no portability is lost.
# NOTE: wander_resolver.c uses dl_iterate_phdr unconditionally, which is a GNU extension.
libwander = library('wander', libwander_src, include_directories : libwander_inc, dependencies : [ libdweller_dep, libdl ], c_args : ['-D_GNU_SOURCE'])
libwander_dep = declare_dependency(include_directories : libwander_inc, link_with : libwander)
