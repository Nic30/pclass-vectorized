#!/bin/bash
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

# sudo apt install libunbound-dev

function prepare_ovs() {
	local ovs_dir="$1"
	local meson_file="$2"
	local pre_build="$3"

	if [ ! -d "$ovs_dir" ]; then
		if [ -d "$pre_build" ]; then
			cp "$pre_build" "$ovs_dir" -r
		else
			git clone https://github.com/openvswitch/ovs.git "$ovs_dir"
			pushd "$ovs_dir"
			git checkout v2.10.1
			popd
		fi
	fi

	cp "$meson_file" "$ovs_dir/meson.build"
	pushd "$ovs_dir"
		./boot.sh
		./configure

		# make to actually generate files
		make -j $CORES

		git apply ../0001-c-17-compatibility.patch
		git apply ../0002-MINIFLOW_GET_TYPE-ignore-asserts-c-compatibility.patch
		git apply ../0003-export-internals-of-ovs-dpcls.patch
	popd
}

prepare_ovs "ovs" "ovs_meson.build"
prepare_ovs "ovs_pcv" "ovs_pcv_meson.build" "ovs"
for filename in ovs_api/*; do
	l_name="ovs_pcv/lib/$(basename $filename)"
	rm ovs_pcv/lib/$(basename $filename)
	echo "linking $filename to ovs"
	ln -s "$PWD/$filename" "$l_name"
done
