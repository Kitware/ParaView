# trace generated using paraview version 5.12.0-RC2-419-g91dcd7f581
#import paraview
#paraview.compatibility.major = 5
#paraview.compatibility.minor = 12

#### import the simple module from the paraview
from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot
import os.path

LoadPalette("BlueGrayBackground")

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# import file
ImportView(os.path.join(vtkGetDataRoot(), 'Testing/Data/glTF/NestedRings/NestedRings.glb'), view=renderView1, NodeSelectors=['/assembly/Axle', '/assembly/OuterRing/Torus002', '/assembly/OuterRing/MiddleRing/InnerRing'])

# get active source.
innerRing_Torus001 = GetActiveSource()

# get display properties
innerRing_Torus001Display = GetDisplayProperties(innerRing_Torus001, view=renderView1)

# get layout
layout1 = GetLayout()

#--------------------------------
# saving layout sizes for layouts

# layout/tab size in pixels
layout1.SetSize(300, 300)

renderView1.ResetActiveCameraToPositiveY()

# reset view to fit data
renderView1.ResetCamera(False, 0.9)

# Render all views to see them appears
RenderAllViews()

## Interact with the view, usefull when running from pvpython
# Interact()

# compare with baseline image
import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
  exit(1)
baseline_file = os.path.join(baselinePath, "glTFImporterNestedRings.png")

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView1.GetRenderWindow(), baseline_file)
Testing.interact()
