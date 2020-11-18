# filename: __init__.py
# used to test that Catalyst can load Packages
# correctly.

from paraview.simple import *
from paraview import print_info

# print start marker
print_info("begin '%s'", __name__)

tracker = {}
def count(f):
    def wrapper(*args, **kwargs):
        global tracker
        c = tracker.get(f.__name__, 0)
        tracker[f.__name__] = c+1
        return f(*args, **kwargs)
    return wrapper

@count
def catalyst_initialize():
    print_info("in '%s::catalyst_initialize'", __name__)

@count
def catalyst_execute(info):
    print_info("in '%s::catalyst_execute'", __name__)

@count
def catalyst_finalize():
    print_info("in '%s::catalyst_finalize'", __name__)
    global tracker
    assert tracker["catalyst_initialize"] == 1
    assert tracker["catalyst_finalize"] == 1
    assert tracker["catalyst_execute"] >= 1
    print_info("All's ok")

# print end marker
print_info("end '%s'", __name__)
