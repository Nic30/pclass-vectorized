project('packet_classifiers_vectorized', ['c', 'cpp'], default_options : ['cpp_std=c++17', 'c_std=c11', 'debug=true'])

PCV_OPT = [
 '-Wfatal-errors',
 '-m64',
 '-march=haswell',
 '-mavx2',
]

if get_option('debug')
   PCV_OPT += ['-O3', '-DNDEBUG']
else
   PCV_OPT += ['-O0', '-g3']
endif

subdir('src')
subdir('tests')
subdir('benchmarks')
