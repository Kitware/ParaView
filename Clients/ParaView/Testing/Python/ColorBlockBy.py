#### import the simple module from the paraview
from paraview.simple import *

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

LoadPalette("BlueGrayBackground")

# create a new 'IOSS Reader'
from paraview.vtk.util.misc import vtkGetDataRoot
import os.path

canex2 = IOSSReader(registrationName='can.ex2', FileName=os.path.join(vtkGetDataRoot(), 'Testing/Data/can.ex2'))

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
canex2Display = Show(canex2, renderView1, 'UnstructuredGridRepresentation')

# trace defaults for the display properties.
canex2Display.Representation = 'Surface'

# reset view to fit data
renderView1.ResetCamera(False, 0.9)

# update the view to ensure updated data information
renderView1.Update()

# set scalar coloring
ColorBy(canex2Display, ('FIELD', 'vtkBlockColors'))

# hide color bar/color legend
canex2Display.SetScalarBarVisibility(renderView1, False)

# get color transfer function/color map for 'vtkBlockColors'
vtkBlockColorsLUT = GetColorTransferFunction('vtkBlockColors')

# get opacity transfer function/opacity map for 'vtkBlockColors'
vtkBlockColorsPWF = GetOpacityTransferFunction('vtkBlockColors')

# get 2D transfer function for 'vtkBlockColors'
vtkBlockColorsTF2D = GetTransferFunction2D('vtkBlockColors')

# set block scalar coloring
ColorBlocksBy(canex2Display, ['/IOSS/element_blocks/block_2'], ('POINTS', 'ACCL', 'X'))

# rescale block color and/or opacity maps used to exactly fit the current data range
canex2Display.RescaleBlocksTransferFunctionToDataRange(['/IOSS/element_blocks/block_2'], False, True)

# get color transfer function/color map for 'ACCl'
blockACCLLUT = GetBlockColorTransferFunction('/IOSS/element_blocks/block_2', 'ACCL')

# show block color bar/color legend
canex2Display.SetBlocksScalarBarVisibility(renderView1, ['/IOSS/element_blocks/block_2'], True)

blockACCLLUT.ApplyPreset("Cool to Warm", True)

# reset active camera to negative y
renderView1.ResetActiveCameraToNegativeY()

# reset view to fit data
renderView1.ResetCamera(False, 0.9)

import os
import sys

try:
    baselineIndex = sys.argv.index('-B') + 1
    baselinePath = sys.argv[baselineIndex]
except:
    print("Could not get baseline directory. Test failed.")
baseline_file = os.path.join(baselinePath, "ColorBlockBy.png")
from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir

Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView1.GetRenderWindow(), baseline_file)
Testing.interact()
