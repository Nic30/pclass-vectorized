
boost_deps = dependency('boost', modules: ['unit_test_framework', ])
tests = [
  ['b_tree', 'simple test of b-tree impl'],
  ['b_tree_key_iterator', 'b-tree key iterator test'],
  ['b_tree_collision_check', 'test of methods for collision checking between the values stored in tree'],
  ['b_tree_compression', 'tests for the tree path compression'],
  ['mempool', 'tests of static memory mempool'],
  ['classbench_rule_parser', 'tests for functionality of classbench rule parser'],
  ['layered_b_tree', 'tests for layering mechanism of the of b-tree impl'],
  ['weighted_interval_scheduling_solver', 'tests for the weighted interval scheduling solver'],
  ['dimension_order_resolver', 'tests for the dimension order solver which is used to resolve structure of the trees'],
  ['partition_sort_classifier', 'tests for the partition sort packet classifer itself'],
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