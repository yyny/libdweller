project('libdweller', 'c', default_options : ['c_std=c89'])

libdweller_src = files('src/dwarf.c')
libdweller_inc = include_directories('include')

libdweller = shared_library('dweller', libdweller_src, include_directories : libdweller_inc)
libdweller_dep = declare_dependency(include_directories : libdweller_inc, link_with : libdweller)

subdir('dwarfdump')
subdir('examples')
subdir('tests')
