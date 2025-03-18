import paraview.simple as smp
from paraview.vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
import os, sys

data_dir = vtkGetDataRoot() + "/Testing/Data/"

worldObject = smp.WavefrontOBJReader(registrationName='WorldWithTexture.obj', FileName=os.path.join(data_dir, 'WorldWithTexture.obj'))
renderView = smp.GetActiveViewOrCreate('RenderView')
worldObjectDisplay = smp.Show(worldObject, renderView, 'GeometryRepresentation')

texture = smp.FindTextureOrCreate(registrationName='WorldWithTexture', filename=os.path.join(data_dir, 'WorldWithTexture.png'))
worldObjectDisplay.Texture = texture

renderView.Update()

try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")
  exit(1)

baseline_file = os.path.join(baselinePath, "LoadAndApplyTexture.png")
from paraview.vtk.test import Testing
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView.GetRenderWindow(), baseline_file)
Testing.interact()
