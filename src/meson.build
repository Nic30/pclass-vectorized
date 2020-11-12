
src = [
	'rule_parser/classbench_rule_parser.cpp',
	'rule_parser/rule.cpp',
	'rule_parser/trace_tools.cpp',
	'partition_sort/b_tree_search.cpp',
	'utils/debug_timer.cpp',
	'utils/benchmark_common.cpp',
]

PCV_EXTRA_CXX_ARGS = PCV_OPT + [
 	'-Wall',
 	'-std=c++17',
]
PCV_EXTRA_C_ARGS = PCV_EXTRA_CXX_ARGS

_pcv_inc = include_directories('../include/')
_pcv_lib = library(
	'pcv',
    src,
    include_directories : [_pcv_inc, ],
    install : true,
    cpp_args: PCV_EXTRA_CXX_ARGS,
    c_args: PCV_EXTRA_C_ARGS,
)
install_subdir('../include/pcv', install_dir : 'include')
pcv_dep = declare_dependency(
  link_with : _pcv_lib,
  include_directories : _pcv_inc
)
