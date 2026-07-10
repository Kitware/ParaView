# script-version: 2.0
"""Catalyst V2 analysis pipeline verifying that the fides_conduit 'grid'
channel produced a vtkCellGrid (DG hexahedra) with the expected cell count and
cell-grid attributes."""
from paraview.simple import *
from vtkmodules.vtkCommonDataModel import vtkCellGrid, vtkPartitionedDataSetCollection

print("executing fides_conduit cellgrid verifier")

producer = TrivialProducer(registrationName="grid")

n_exec = 0


def catalyst_execute(info):
    global n_exec
    producer.UpdatePipeline()

    # Data information crosses the client/server boundary and reports the
    # cell-grid type and cell count.
    di = producer.GetDataInformation()
    assert di.GetDataClassName() == "vtkCellGrid", di.GetDataClassName()
    assert di.GetNumberOfCells() == 2, di.GetNumberOfCells()

    # vtkCellGrid does not round-trip through servermanager.Fetch, so read the
    # data object straight off the client-side producer (in Catalyst the data
    # is local) to inspect its cell-grid attributes.
    dobj = producer.GetClientSideObject().GetOutputDataObject(0)
    assert isinstance(dobj, vtkPartitionedDataSetCollection), type(dobj)
    cg = dobj.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
    assert isinstance(cg, vtkCellGrid), "expected vtkCellGrid, got %s" % type(cg).__name__

    ids = list(cg.GetUnorderedCellAttributeIds())
    attrs = sorted(cg.GetCellAttributeById(i).GetName().Data() for i in ids)
    print("  step={} time={} cells={} attrs={}".format(
        info.timestep, info.time, cg.GetNumberOfCells(), attrs))
    assert cg.GetNumberOfCells() == 2, cg.GetNumberOfCells()
    assert "temperature" in attrs, attrs
    assert "shape" in attrs, attrs
    n_exec += 1


def catalyst_finalize():
    assert n_exec == 3, "expected 3 executes, got %d" % n_exec
    # ctest matches this string to confirm the test passed.
    print("All ok")
