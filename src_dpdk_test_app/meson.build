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


pcv_dpdk_exe = executable('pcv_dpdk',
	src,
	dependencies: [dpdk_deps, pcv_dep],
    #c_args: EXTRA_C_ARGS,
    #include_directories: pcv_incldir
)
#test(b[0], benchmark_exe, workdir : meson.source_root(), args : b[2])