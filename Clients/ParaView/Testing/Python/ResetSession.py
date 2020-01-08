# trace generated using paraview version 5.5.2-506-g3b566ba

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Cone'
cone1 = Cone()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1378, 1006]

# show data in view
cone1Display = Show(cone1, renderView1)

# trace defaults for the display properties.
cone1Display.Representation = 'Surface'
cone1Display.ColorArrayName = [None, '']
cone1Display.OSPRayScaleFunction = 'PiecewiseFunction'
cone1Display.SelectOrientationVectors = 'None'
cone1Display.ScaleFactor = 0.1
cone1Display.SelectScaleArray = 'None'
cone1Display.GlyphType = 'Arrow'
cone1Display.GlyphTableIndexArray = 'None'
cone1Display.GaussianRadius = 0.005
cone1Display.SetScaleArray = [None, '']
cone1Display.ScaleTransferFunction = 'PiecewiseFunction'
cone1Display.OpacityArray = [None, '']
cone1Display.OpacityTransferFunction = 'PiecewiseFunction'
cone1Display.DataAxesGrid = 'GridAxesRepresentation'
cone1Display.SelectionCellLabelFontFile = ''
cone1Display.SelectionPointLabelFontFile = ''
cone1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
cone1Display.DataAxesGrid.XTitleFontFile = ''
cone1Display.DataAxesGrid.YTitleFontFile = ''
cone1Display.DataAxesGrid.ZTitleFontFile = ''
cone1Display.DataAxesGrid.XLabelFontFile = ''
cone1Display.DataAxesGrid.YLabelFontFile = ''
cone1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
cone1Display.PolarAxes.PolarAxisTitleFontFile = ''
cone1Display.PolarAxes.PolarAxisLabelFontFile = ''
cone1Display.PolarAxes.LastRadialAxisTextFontFile = ''
cone1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# reset view to fit data
renderView1.ResetCamera()

# update the view to ensure updated data information
renderView1.Update()

ResetSession()

# create a new 'Box'
box1 = Box()

# get active view
renderView1_1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1_1.ViewSize = [1378, 1006]

# show data in view
box1Display = Show(box1, renderView1_1)

# trace defaults for the display properties.
box1Display.Representation = 'Surface'
box1Display.ColorArrayName = [None, '']
box1Display.OSPRayScaleArray = 'Normals'
box1Display.OSPRayScaleFunction = 'PiecewiseFunction'
box1Display.SelectOrientationVectors = 'None'
box1Display.ScaleFactor = 0.1
box1Display.SelectScaleArray = 'None'
box1Display.GlyphType = 'Arrow'
box1Display.GlyphTableIndexArray = 'None'
box1Display.GaussianRadius = 0.005
box1Display.SetScaleArray = ['POINTS', 'Normals']
box1Display.ScaleTransferFunction = 'PiecewiseFunction'
box1Display.OpacityArray = ['POINTS', 'Normals']
box1Display.OpacityTransferFunction = 'PiecewiseFunction'
box1Display.DataAxesGrid = 'GridAxesRepresentation'
box1Display.SelectionCellLabelFontFile = ''
box1Display.SelectionPointLabelFontFile = ''
box1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
box1Display.ScaleTransferFunction.Points = [-1.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
box1Display.OpacityTransferFunction.Points = [-1.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
box1Display.DataAxesGrid.XTitleFontFile = ''
box1Display.DataAxesGrid.YTitleFontFile = ''
box1Display.DataAxesGrid.ZTitleFontFile = ''
box1Display.DataAxesGrid.XLabelFontFile = ''
box1Display.DataAxesGrid.YLabelFontFile = ''
box1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
box1Display.PolarAxes.PolarAxisTitleFontFile = ''
box1Display.PolarAxes.PolarAxisLabelFontFile = ''
box1Display.PolarAxes.LastRadialAxisTextFontFile = ''
box1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# reset view to fit data
renderView1_1.ResetCamera()

# update the view to ensure updated data information
renderView1_1.Update()

#### saving camera placements for all active views

# current camera placement for renderView1_1
renderView1_1.CameraPosition = [0.0, 0.0, 3.3460652149512318]
renderView1_1.CameraParallelScale = 0.8660254037844386

Render()

# compare with baseline image
import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
  exit(1)
baseline_file = os.path.join(baselinePath, "ResetSession.png")

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(GetActiveView().GetRenderWindow(), baseline_file,
                     threshold=25)
Testing.interact()
