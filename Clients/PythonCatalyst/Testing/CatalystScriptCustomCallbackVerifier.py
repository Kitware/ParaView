# script-version: 2.0

r"""A test to ensure all custom functions supported by Catalyst
Scripts V2 are called appropriately"""

counters = {}

def update_counter(f):
    def wrapper(*args, **kwargs):
        global counters
        count = counters.get(f.__name__, 0)
        counters[f.__name__] = count + 1
        return f(*args, **kwargs)
    return wrapper

@update_counter
def catalyst_initialize():
    pass

@update_counter
def catalyst_execute(info):
    pass

@update_counter
def RequestDataDescription(dataDescription):
    """this is an intentionally undocumented callback.
    don't rely on it"""
    for i in range(dataDescription.GetNumberOfInputDescriptions()):
        dataDescription.GetInputDescription(i).GenerateMeshOn()

@update_counter
def catalyst_finalize():
    global counters
    assert counters["catalyst_initialize"] == 1
    assert counters["catalyst_finalize"] == 1
    # vtkCPProcessor calls RequestDataDescription a bit too many times
    # not changing that right now. it'll be all history in the
    # the new Catalyst API anyways.
    assert counters["RequestDataDescription"] >= 20
    assert counters["catalyst_execute"] == 20

    # this is needed since it's used to confirm that the test passed.
    print("All ok")
