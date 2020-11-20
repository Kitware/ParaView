# Tests the simulation to ensure it is generating a time-varying dataset.
# script-version: 2.0

from paraview import catalyst
options = catalyst.Options()

# ensure not extracts get saved anywhere.
options.ExtractsOutputDirectory = ""
options.GlobalTrigger = "TimeStep"

source = None
ranges = []

def catalyst_initialize():
    """setup visualization pipeline"""
    global source
    from paraview.simple import Wavelet
    source = Wavelet(registrationName="input")

def catalyst_execute(info):
    """do stuff for each timestep"""
    global source, ranges
    time = info.time
    source.UpdatePipeline(time)
    data_range = source.PointData["RTData"].GetRange()
    ranges.append(data_range)

def catalyst_finalize():
    """validate results"""
    global ranges
    if len(ranges) < 2:
        raise RuntimeError("Test Failed! More than 1 timestep expected!")

    print("ranges:", ranges)
    for idx in range(1, len(ranges)-1):
        if ranges[idx-1] == ranges[idx]:
            raise RuntimeError(\
               "Test failed! Two consequetive timesteps produced same data!")
    print("All ok")
