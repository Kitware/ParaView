#### import the simple module from the paraview
import paraview

from paraview.simple import *
from paraview import smtesting
import os.path

smtesting.ProcessCommandLineArguments()

# create a new 'Wavelet'
wavelet1 = Wavelet()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
renderView1.ViewSize = [400, 400]

# show data in view
wavelet1Display = Show(wavelet1, renderView1)

# reset view to fit data
renderView1.ResetCamera()

# create a new 'Contour'
contour1 = Contour(Input=wavelet1)
contour1.ContourBy = ['POINTS', 'RTData']
contour1.Isosurfaces = [97.222075, 157.09105, 216.96002500000003, 276.829]
contour1.ComputeScalars = 1
contour1.PointMergeMethod = 'Uniform Binning'

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')

# show data in view
contour1Display = Show(contour1, renderView1)
# trace defaults for the display properties.
contour1Display.ColorArrayName = ['POINTS', 'RTData']
contour1Display.LookupTable = rTDataLUT

# save data
plyfilename = os.path.join(smtesting.TempDir, "PLYWriterData.ply")
SaveData(plyfilename,
        proxy=contour1, EnableColoring=1,
# These properties need not be specified if being set to the coloring state of
# the input in the active view.
#        ColorArrayName=['POINTS', 'RTData'],
#        LookupTable=rTDataLUT
)

# save data as a time series with time step padding
plyfilenamewithpadding = os.path.join(smtesting.TempDir, "PLYWriterDataWithPadding.ply")
SaveData(plyfilenamewithpadding,
        proxy=contour1, EnableColoring=1,
# These properties need not be specified if being set to the coloring state of
# the input in the active view.
#        ColorArrayName=['POINTS', 'RTData'],
#        LookupTable=rTDataLUT
    WriteTimeSteps=1,
    Filenamesuffix='_%.3d'
)

# destroy contour1
Delete(contour1)
del contour1

# destroy wavelet1
Delete(wavelet1)
del wavelet1

# create a new 'PLY Reader'
fooply = PLYReader(FileNames=[plyfilename])

# show data in view
fooplyDisplay = Show(fooply, renderView1)
fooplyDisplay.MapScalars = 0

# reset view to fit data
Render()
ResetCamera()

if not smtesting.DoRegressionTesting(renderView1.SMProxy):
    raise smtesting.TestError ('Test failed.')

splitname = plyfilenamewithpadding.split('.')
newfilename = splitname[0]+'_000.'+splitname[1]
fooply.FileNames=[newfilename]
fooply.UpdatePipeline()

# show data in view
fooplyDisplay = Show(fooply, renderView1)
fooplyDisplay.MapScalars = 0

# reset view to fit data
Render()
ResetCamera()

if not smtesting.DoRegressionTesting(renderView1.SMProxy):
    raise smtesting.TestError ('Test failed.')
