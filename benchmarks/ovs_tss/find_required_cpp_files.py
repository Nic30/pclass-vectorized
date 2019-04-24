import sys
import CppHeaderParser

OVS = "build/meson.debug.linux.x86_64.1/benchmarks/ovs_tss/ovs/"
INCLUDE_DIRS = [OVS + "/include", OVS]

required_files = set()
unresolved_files = [OVS + "lib/classifier.cpp"]

def find_file_in_incl_dirs(file, ):
    raise NotImplementedError()


while unresolved_files:
    f = unresolved_files.pop()
    
    headers = CppHeaderParser.CppHeader().includes
    for h in headers:
        if ()
