project('libdweller', 'c', default_options : ['c_std=c89'])

src = files('src/dwarf.c')
inc = include_directories('include')

libdweller = shared_library('dweller', src, include_directories : inc)
libdweller_dep = declare_dependency(include_directories : inc, link_with : libdweller)

subdir('dwarfdump')
subdir('examples')
subdir('tests')
