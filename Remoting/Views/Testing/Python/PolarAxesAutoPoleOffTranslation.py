from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

sphere1 = Sphere(registrationName='Sphere1')
renderView1 = GetActiveViewOrCreate('RenderView')
sphere1Display = Show(sphere1, renderView1, 'GeometryRepresentation')
sphere1Display.Representation = 'Surface'
renderView1.ResetCamera(False, 0.9)
renderView1.Update()

transform1 = Transform(registrationName='Transform1', Input=sphere1)
transform1.Transform.Translate = [5.0, 2.0, 1.0]
transform1Display = Show(transform1, renderView1, 'GeometryRepresentation')
transform1Display.Representation = 'Surface'
Hide(sphere1, renderView1)

renderView1.Update()
renderView1.ResetCamera(False, 0.9)

HideInteractiveWidgets(proxy=transform1.Transform)

transform1Display.PolarAxes.Visibility = 1
transform1Display.PolarAxes.AutoPole = 0
transform1Display.PolarAxes.Translation = [5.0, 2.0, 1.0]

# Properties modified on transform1Display.DataAxesGrid
transform1Display.DataAxesGrid.GridAxesVisibility = 1

# Properties modified on transform1Display.DataAxesGrid
transform1Display.DataAxesGrid.UseCustomBounds = 1
transform1Display.DataAxesGrid.CustomBounds = [0.0, 5.0, 0.0, 2.0, 0.0, 1.0]

renderView1.ResetCamera(False, 0.9)

layout1 = GetLayout()
layout1.SetSize(300, 300)

renderView1.InteractionMode = '2D'
renderView1.CameraPosition = [5.511463174995544, 4.012063174995544, 27.256085151235204]
renderView1.CameraFocalPoint = [5.511463174995544, 4.012063174995544, 0.7498]
renderView1.CameraParallelScale = 6.86033141205782

Render(renderView1)

import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")
baseline_file = os.path.join(baselinePath, "PolarAxesAutoPoleOffTranslation.png")
from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView1.GetRenderWindow(), baseline_file, threshold=40)
Testing.interact()
