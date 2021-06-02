project('libdweller', 'c', 'cpp', default_options : ['c_std=c11'])

libdweller_src = files('src/dwarf.c')
libdweller_inc = include_directories('include')

libdweller = library('dweller', libdweller_src, include_directories : libdweller_inc, c_args : ['-g'])
libdweller_dep = declare_dependency(include_directories : libdweller_inc, link_with : libdweller)

subdir('libwander')
subdir('dwarfdump')
subdir('examples')
subdir('tests')
