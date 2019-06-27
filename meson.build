project('packet_classifiers_vectorized', ['c', 'cpp'], default_options : ['cpp_std=c++17', 'c_std=c11'])

PCV_OPT = [
 '-m64',
 '-march=haswell',
 '-mavx2',
 '-O3',
 #'-g',
]

subdir('src')
subdir('tests')
subdir('benchmarks')
