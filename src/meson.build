
src = [
	'rule_parser/classbench_rule_parser.cpp',
	'rule_parser/rule.cpp',
	'rule_parser/trace_tools.cpp',
	'utils/debug_timer.cpp',
	'utils/benchmark_common.cpp',
]

EXTRA_CXX_ARGS = [
 	#'-O3',
 	'-g',
 	'-Wall',
 	'-m64',
 	'-mavx2',
 	'-march=haswell',
 ]

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
