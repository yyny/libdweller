cc = meson.get_compiler('c')

libwander_src = files('src/wander.c', 'src/wander_platform.c', 'src/wander_resolver.c', 'src/wander_printer.c')
libwander_inc = include_directories('include')

libdl = cc.find_library('dl')
libwander = library('wander', libwander_src, include_directories : libwander_inc, dependencies : [ libdweller_dep, libdl ], c_args : ['-D_XOPEN_SOURCE=700'])
libwander_dep = declare_dependency(include_directories : libwander_inc, link_with : libwander)
