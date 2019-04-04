project('packet_classifiers_vectorized', ['c', 'cpp'], default_options : ['cpp_std=c++17', ])

subdir('src')
subdir('tests')
subdir('benchmarks')

build_dpdk_test_app = get_option('dpdk_test_app')
if build_dpdk_test_app
	subdir('src_dpdk_test_app')
endif