
boost_deps = dependency('boost', modules: ['unit_test_framework', ])


foreach t : tests
	test_exe = executable(t[0]  + '_test',
		[t[0] + '_test.cpp'],
		dependencies: [boost_deps, dpdk_dep],
		link_with: [pcv_lib, ],
		cpp_args: EXTRA_CXX_ARGS,
        c_args: EXTRA_C_ARGS,
        #include_directories: pcv_incldir
        )
	test(t[1], test_exe, workdir : meson.source_root())
endforeach