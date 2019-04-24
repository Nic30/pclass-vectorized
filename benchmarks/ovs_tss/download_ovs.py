
import sys
import os
from os import chdir

from subprocess import check_call


def git(*args):
    return check_call(['git'] + list(args))

if __name__ == "__main__":
    dst = sys.argv[1]
    if os.path.isdir(dst):
        chdir(dst)
        #git("pull")
    else:
        git("clone", "https://github.com/openvswitch/ovs.git", dst)
        chdir(dst)
        check_call(["./boot.sh"])
        check_call(["./configure"])
        
