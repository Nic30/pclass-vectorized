#dpdk_deps = dependency('libdpdk', modules: ['acl', 'lpm', 'hash', 'cryptodev'], static:true)

src = [
	#'classbench_rule_parser.cpp',
	#'classbench_rule_parser.h',
	#'dpdk_acl_container.cpp',
	#'dpdk_acl_container.h',
	#'dpdk_acl_field_defs.cpp',
	#'dpdk_acl_field_defs.h',
	#'dpdk_acl_rule_dump.h',
	#'log.h',
	#'packet_classifiers_vectorized.cpp',
	
	'rule_parser/classbench_rule_parser.cpp',
	'rule_parser/rule.cpp',
	'rule_parser/trace_tools.cpp',
	
]

EXTRA_CXX_ARGS = [
 	#'-O3',
 	'-Wall',
 	'-m64',
 	'-mavx2',
 	'-march=haswell',
 ]
#executable('packet_classifiers_vectorized', src,
#	dependencies: [dpdk_deps, ],
#	cpp_args: EXTRA_CXX_ARGS)


_pcv_inc = include_directories('../include/')
_pcv_lib = library(
	'pcv',
    src,
    include_directories : [_pcv_inc, ],
    install : true,
    cpp_args: EXTRA_CXX_ARGS,
)
install_subdir('../include/pcv', install_dir : 'include')
pcv_dep = declare_dependency(
  link_with : _pcv_lib,
  include_directories : _pcv_inc
)
	