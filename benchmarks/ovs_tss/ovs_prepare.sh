#!/usr/bin/bash

if [ ! -d "ovs" ]; then
	git clone https://github.com/openvswitch/ovs.git
fi

cd ovs
./boot.sh
./configure

CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
# make to actually generate files
make -j $CORES

cp ../ovs_meson.build meson.build
git apply ../0001_define_typeof.patch

