dpdk_deps = dependency('libdpdk', modules: ['acl', 'lpm', 'hash', 'cryptodev'], static:true)

src = [
	'dpdk_acl_container.cpp',
	'dpdk_acl_container.h',
	'dpdk_acl_field_defs.cpp',
	'dpdk_acl_field_defs.h',
	'dpdk_acl_rule_dump.h',
	'log.h',
	'dpdk_test_app_main.cpp',	
]


pcv_dpdk_exe = executable('1_tree_lookup_dpdk',
	src,
	dependencies: [dpdk_deps, pcv_dep],
	cpp_args: EXTRA_CXX_ARGS + ['-O3', ],
    #c_args: EXTRA_C_ARGS,
)
#test(b[0], pcv_dpdk_exe, workdir : meson.source_root(), args : b[2])