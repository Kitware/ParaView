# script-version: 2.0
"""Catalyst V2 analysis pipeline verifying that the fides_conduit 'grid'
channel produced the expected unstructured grid (two hexahedra with a
per-point 'temperature' field, and a per-step time)."""
from paraview.simple import *

print("executing fides_conduit unstructured verifier")

# registrationName must match the Catalyst channel name from the mini-app.
producer = TrivialProducer(registrationName="grid")

n_exec = 0


def catalyst_execute(info):
    global n_exec
    producer.UpdatePipeline()
    di = producer.GetDataInformation()
    ncells = di.GetNumberOfCells()
    npts = di.GetNumberOfPoints()
    trange = producer.PointData["temperature"].GetRange(0)
    print("  step={} time={} cells={} points={} temp_range={}".format(
        info.timestep, info.time, ncells, npts, trange))

    assert ncells == 2, "expected 2 cells, got %d" % ncells
    assert npts == 12, "expected 12 points, got %d" % npts
    # temperature = (arange(12) + step) * 10 -> range [step*10, (11+step)*10]
    assert abs(trange[0] - info.timestep * 10.0) < 1e-6, trange
    assert abs(trange[1] - (11 + info.timestep) * 10.0) < 1e-6, trange
    n_exec += 1


def catalyst_finalize():
    assert n_exec == 3, "expected 3 executes, got %d" % n_exec
    # ctest matches this string to confirm the test passed.
    print("All ok")
