#!/usr/bin/python3

import subprocess
import sys

def call(cmd):
    out = ""
    try:
        out = subprocess.check_output(cmd, shell=True, universal_newlines=True, stderr=subprocess.STDOUT)
    except Exception as e:
        out = e.output
    return out

libflag = sys.argv[1]
out = call(f"gcc -{libflag}")
if "undefined reference to `main'" not in out:
    # try installing common packages for the two libs used in your workflow
    if libflag == "lelf":
        call("sudo apt-get update && sudo apt-get install -y libelf-dev zlib1g-dev")
    elif libflag == "lcheck_pic":
        call("sudo apt-get update && sudo apt-get install -y check")

    out = call(f"gcc -{libflag}")
    if "undefined reference to `main'" not in out:
        raise Exception(out)
