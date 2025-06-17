from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot
from os.path import realpath, join, dirname

scriptdir = dirname(realpath(__file__))
statefile = join(scriptdir, "StateWithDatasets.pvsm")
data_dir = vtkGetDataRoot() + "/Testing/Data/"

# ------------------------------------------------------------------------------------
# old style state loading calls.

LoadState(
    statefile,
    LoadStateDataFileOptions="Search files under specified directory",
    DataDirectory=data_dir,
    OnlyUseFilesInDataDirectory=1,
)
ResetSession()

LoadState(
    statefile,
    LoadStateDataFileOptions="Choose File Names",
    canex2FileName=[data_dir + "can.ex2"],
    datasetFileName=[data_dir + "disk_out_ref.ex2"],
    timeseriesFileName=[
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0000.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0001.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0002.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0003.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0004.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0005.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0006.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0007.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0008.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0009.vtp",
        data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0010.vtp",
    ],
)

ResetSession()

# ------------------------------------------------------------------------------------
# new style state loading calls.
LoadState(statefile, data_directory=data_dir, restrict_to_data_directory=True)
ResetSession()

# load state using specified data files
LoadState(
    statefile,
    filenames=[
        {
            "name": "can.ex2",
            "id": "11470",
            "FileName": data_dir + "can.ex2",
        },
        {
            "name": "dataset",
            "id": "11844",
            "FileName": data_dir + "disk_out_ref.ex2",
        },
        {
            "name": "timeseries",
            "id": "12168",
            "FileName": [
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0000.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0001.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0002.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0003.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0004.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0005.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0006.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0007.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0008.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0009.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0010.vtp",
            ],
        },
    ],
)
ResetSession()

# load state using specified data files without proxy ids from the statefile
LoadState(
    statefile,
    filenames=[
        {
            "name": "can.ex2",
            "FileName": data_dir + "can.ex2",
        },
        {
            "name": "dataset",
            "FileName": data_dir + "disk_out_ref.ex2",
        },
        {
            "name": "timeseries",
            "FileName": [
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0000.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0001.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0002.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0003.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0004.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0005.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0006.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0007.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0008.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0009.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0010.vtp",
            ],
        },
    ],
)
ResetSession()

# load state using specified data files (swapped)
LoadState(
    statefile,
    filenames=[
        {
            "name": "can.ex2",
            "id": "11470",
            "FileName": data_dir + "disk_out_ref.ex2",
        },
        {
            "name": "dataset",
            "id": "11844",
            "FileName": data_dir + "can.ex2",
        },
        {
            "name": "timeseries",
            "id": "12168",
            "FileName": [
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0000.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0001.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0002.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0003.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0004.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0005.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0006.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0007.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0008.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0009.vtp",
                data_dir + "dualSphereAnimation/dualSphereAnimation_P00T0010.vtp",
            ],
        },
    ],
)

names = set([x[0] for x in GetSources().keys()])
# the 'can.ex2' is renamed, but others which have been manually change in the state file
# should remain unchanged.
assert names == set(["dataset", "disk_out_ref.ex2", "timeseries"])
