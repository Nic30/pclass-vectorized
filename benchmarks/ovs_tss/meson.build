prog_python = import('python').find_installation('python3')

ovs_files = custom_target(
    'ovs_files',
    output : ['ovs'],
    input : 'download_ovs.py',
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
)


srcs = ['n_tree_lookup.cpp', ovs_files, './ovs/lib/cmap.c']

benchmark_exe = executable('n_tree_lookup',
    srcs,
    dependencies: [pcv_dep],
    cpp_args: EXTRA_CXX_ARGS + ['-O3', '-Dtypeof=__typeof__'],
    include_directories : ['./ovs/', './ovs/include']
    #c_args: EXTRA_C_ARGS,
)
test('n_tree_lookup', benchmark_exe, workdir : meson.source_root(),
	args : ['tests/data/acl1_100', '4096', '10000000', '0'])