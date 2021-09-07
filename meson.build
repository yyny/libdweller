project('libdweller', 'c', 'cpp', version : '0.1.0', default_options : ['c_std=c11', 'warning_level=3'])

cc = meson.get_compiler('c')
if cc.has_argument('-fno-delete-null-pointer-checks')
    add_project_arguments([ '-fno-delete-null-pointer-checks' ], language : 'c')
endif

libdweller_src = files('src/dwarf.c')
libdweller_inc = include_directories('include')

libdweller = library('dweller', libdweller_src, include_directories : libdweller_inc, c_args : ['-g'])
libdweller_dep = declare_dependency(include_directories : libdweller_inc, link_with : libdweller)

subdir('libwander')
subdir('dwarfdump')
subdir('examples')
subdir('tests')
