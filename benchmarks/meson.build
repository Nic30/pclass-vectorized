
benchmarks = [
  ['1_tree_lookup', ['1_tree_lookup.cpp'], ['tests/data/acl1_100', '4096', '10000000']],
  
]

foreach b : benchmarks
	benchmark_exe = executable(t[0],
		b[1],
		dependencies: [pcv_dep],
		cpp_args: EXTRA_CXX_ARGS + ['-O3', ],
        #c_args: EXTRA_C_ARGS,
        #include_directories: pcv_incldir
    )
	test(b[0], benchmark_exe, workdir : meson.source_root(), args : b[2])
endforeach