project('packet_classifiers_vectorized', ['c', 'cpp'], default_options : ['cpp_std=c++17', 'c_std=c11'])

PCV_OPT = [
 '-Wfatal-errors',
 '-m64',
 '-march=haswell',
 '-mavx2',
#'-O3', '-DNDEBUG',
 '-O0', '-g3',
]

subdir('src')
subdir('tests')
subdir('benchmarks')
