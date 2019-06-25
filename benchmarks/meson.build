
_DEF_BENCHMARK_ARGS = ['tests/data/acl1_100', '4096', '1000000', '0']

benchmarks = [
  ['1_tree_lookup', ['1_tree_lookup.cpp'],  [_DEF_BENCHMARK_ARGS, ]],
  ['n_tree_lookup', ['n_tree_lookup.cpp'],  [_DEF_BENCHMARK_ARGS, ]],
  ['list_lookup', ['list/list_lookup.cpp'], [_DEF_BENCHMARK_ARGS, ]],
  ['hash_vs_rb_tree_vs_b_tree', ['hash_vs_rb_tree_vs_b_tree.cpp'], [
       # cls,   rules,  traces,  lookup
	  ['hash', '8192', '32768', '1000000'],
	  ['b_tree', '8192', '32768', '1000000'], ]],
  ['exact_match_rule_gen', ['exact_match_rule_gen.cpp'], [['0', '1024'], ]],
]

# add very short benchmark also to tests to verify that the benchmark is working
foreach b : benchmarks
	benchmark_exe = executable(b[0],
		b[1],
		dependencies: [pcv_dep],
		cpp_args: EXTRA_CXX_ARGS #+ ['-O3', ]
		,
        #c_args: EXTRA_C_ARGS,
    )
    foreach args: b[2]
		test(b[0] + ' ' + args[0], benchmark_exe, workdir : meson.source_root(), args : args)
	endforeach
endforeach

if get_option('dpdk_test_app')
	subdir('dpdk')
endif
if get_option('ovs_test_app')
	subdir('ovs')
endif
