import sys
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        canFile = sys.argv[i+1] + '/Testing/Data/can.ex2'
        diskFile = sys.argv[i+1] + '/Testing/Data/disk_out_ref.ex2'

# create a new 'IOSS Reader'
disk_out_ref = IOSSReader(registrationName='disk_out_ref',
        FileName=[diskFile])
disk_out_ref.ElementBlocks = ['block_1']
disk_out_ref.NodeBlockFields = ['ash3', 'ch4', 'game3', 'h2', 'pres', 'temp', 'v']
disk_out_ref.NodeSets = []
disk_out_ref.SideSets = []

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
disk_out_refDisplay = Show(disk_out_ref, renderView1, 'UnstructuredGridRepresentation')

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
disk_out_refDisplay.ScaleTransferFunction.Points = [0.08047682046890259, 0.0, 0.5, 0.0,
        0.18483947217464447, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
disk_out_refDisplay.OpacityTransferFunction.Points = [0.08047682046890259, 0.0, 0.5, 0.0,
        0.18483947217464447, 1.0, 0.5, 0.0]

# reset view to fit data
renderView1.ResetCamera(False)

# update the view to ensure updated data information
renderView1.Update()

ReplaceReaderFileName(disk_out_ref,
        [canFile], 'FileName')

can = FindSource('can.ex2')

if (can == None):
    raise("Error: new source is not created properly.")
