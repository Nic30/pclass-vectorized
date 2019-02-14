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
]

executable('packet_classifiers_vectorized', src,
	dependencies: [dpdk_deps, ],)
