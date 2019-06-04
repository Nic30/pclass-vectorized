
benchmarks = [
  ['1_tree_lookup', ['1_tree_lookup.cpp'], ['tests/data/acl1_100', '4096', '1000000', '0']],
  ['n_tree_lookup', ['n_tree_lookup.cpp'], ['tests/data/acl1_100', '4096', '1000000', '0']],
  ['exact_match_rule_gen', ['exact_match_rule_gen.cpp'], ['0', '1024']],
]

# add very short benchmark also to tests to verify that the benchmark is working
foreach b : benchmarks
	benchmark_exe = executable(b[0],
		b[1],
		dependencies: [pcv_dep],
		cpp_args: EXTRA_CXX_ARGS + ['-O3', ],
        #c_args: EXTRA_C_ARGS,
    )
	test(b[0], benchmark_exe, workdir : meson.source_root(), args : b[2])
endforeach

if get_option('dpdk_test_app')
	subdir('dpdk')
endif
if get_option('ovs_test_app')
	subdir('ovs')
endif
