dpdk_deps = dependency('libdpdk', modules: ['acl', 'lpm', 'hash', 'cryptodev'], static:true)

src = [
	'classbench_rule_parser.cpp',
	'classbench_rule_parser.h',
	'dpdk_acl_container.cpp',
	'dpdk_acl_container.h',
	'dpdk_acl_field_defs.cpp',
	'dpdk_acl_field_defs.h',
	'dpdk_acl_rule_dump.h',
	'log.h',
	'packet_classifiers_vectorized.cpp',
	
	'partition_sort/bplus_tree.cpp',
	'partition_sort/bplus_tree.h',
	'partition_sort/mempool.h',
]

EXTRA_CXX_ARGS = [
	'-std=c++17',
 	#'-O3',
 	'-Wall',
 	'-m64',
 	'-mavx2',
 	'-march=haswell',
 ]
executable('packet_classifiers_vectorized', src,
	dependencies: [dpdk_deps, ],
	cpp_args: EXTRA_CXX_ARGS)



	