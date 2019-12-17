#!/bin/bash
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
OVS_VERSION=v2.12.0
OVS_GIT=https://github.com/openvswitch/ovs.git

# sudo apt install libunbound-dev

function prepare_ovs() {
	local ovs_dir="$1"
	local meson_file="$2"
	local pre_build="$3"

	if [ ! -d "$ovs_dir" ]; then
		if [ -d "$pre_build" ]; then
			cp "$pre_build" "$ovs_dir" -r
		else
			git clone $OVS_GIT "$ovs_dir"
			pushd "$ovs_dir"
			git checkout $OVS_VERSION
			popd
		fi
	fi

	cp "$meson_file" "$ovs_dir/meson.build"
	pushd "$ovs_dir"
		./boot.sh
		./configure

		# make to actually generate files
		make -j $CORES

		git apply ../0001-gcc-9-compatibility.patch
		git apply ../0002-make-dpcls-api-public.patch
		echo "\n/build/" >> .gitignore
		echo "\n.sc[0-9]*.h\n" >> .gitignore
	popd
}

prepare_ovs "ovs" "ovs_meson.build"
prepare_ovs "ovs_pcv" "ovs_pcv_meson.build" "ovs"
for filename in ovs_api/*; do
	l_name="ovs_pcv/lib/$(basename $filename)"
	[ -f "$l_name" ] && rm "$l_name"
	echo "linking $filename to $l_name"
	ln -s "$PWD/$filename" "$l_name"
done
