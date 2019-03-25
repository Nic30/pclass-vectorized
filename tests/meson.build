
boost_deps = dependency('boost', modules: ['unit_test_framework', ])
tests = [
  ['b_tree', 'simple test of b-tree impl'],
  ['b_tree_key_iterator', 'b-tree key iterator test'],
  ['mempool', 'tests of static memory mempool'],
  ['classbench_rule_parser', 'tests for functionality of classbench rule parser'],
  ['layered_b_tree', 'tests for layering mechanism of the of b-tree impl'],
]

foreach t : tests
	test_exe = executable(t[0]  + '_test',
		[t[0] + '_test.cpp'],
		dependencies: [boost_deps, pcv_dep],
		cpp_args: EXTRA_CXX_ARGS,
        #c_args: EXTRA_C_ARGS,
        #include_directories: pcv_incldir
        )
	test(t[1], test_exe, workdir : meson.source_root())
endforeach