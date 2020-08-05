# Tests the simulation to ensure it is generating a time-varying dataset.

from paraview import catalyst
options = catalyst.Options()

# ensure not extracts get saved anywhere.
options.ExtractsOutputDirectory = ""
options.GlobalTrigger = "Time"

scripts = []
__all__ = scripts + ["options"]

source = None
ranges = []

def catalyst_initialize(dd):
    """setup visualization pipeline"""
    global source
    from paraview.simple import Wavelet
    source = Wavelet(registrationName="input")

def catalyst_coprocess(dd):
    """do stuff for each timestep"""
    global source, ranges
    time = dd.GetTime()
    source.UpdatePipeline(time)
    data_range = source.PointData["RTData"].GetRange()
    ranges.append(data_range)

def catalyst_finalize():
    """validate results"""
    global ranges
    if len(ranges) < 2:
        raise RuntimeError("Test Failed! More than 1 timestep expected!")

    for idx in range(1, len(ranges)-1):
        if ranges[idx-1] == ranges[idx]:
            raise RuntimeError(\
               "Test failed! Two consequetive timesteps produced same data!")
    print("All's well!")
