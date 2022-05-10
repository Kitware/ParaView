__all__ = ["cis", "cdb", "viewers", "pynb", "install"]

import os

#
# new factory function
#
# creates new objects for a consistent high level interface
#
def new( vtype, args ):
    result = None
    if vtype == "cdb":
        if "path" in args:
            from . import cdb
            result = cdb.cdb(args["path"])
    else:
        print("ERROR: unsupported viewer type: {}".format(vtype))

    return result

def path():
    path = os.path.abspath(__file__)
    return os.path.dirname(path)

def version():
    return "1.7.9"
