__all__ = ["cis", "cdb", "viewers", "pynb"]

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
