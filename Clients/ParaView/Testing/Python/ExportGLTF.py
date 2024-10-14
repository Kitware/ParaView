#/usr/bin/env python
from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
import sys, os.path

# This test tests that exporting of a paraview scene correctly exports
# a GLTF object when the GLTF extension is selected.

LoadPalette("BlueGrayBackground")

filename = os.path.join(vtkGetTempDir(), "ExportedGLTF.gltf")
paraview.simple._DisableFirstRenderCameraReset()

# Setup wavelet scene
wavelet1 = Wavelet(registrationName='Wavelet1')
renderView1 = GetActiveViewOrCreate('RenderView')
wavelet1Display = Show(wavelet1, renderView1, 'UniformGridRepresentation')
wavelet1Display.SetRepresentationType('Surface')
ColorBy(wavelet1Display, ('POINTS', 'RTData'))
wavelet1Display.RescaleTransferFunctionToDataRange(True, False)
wavelet1Display.SetScalarBarVisibility(renderView1, False)
renderView1.OrientationAxesVisibility = 0
renderView1.ResetCamera(False, 0.9)
renderView1.Update()

# export view
ExportView(filename, view=renderView1)

# read exported data
exportgltf = glTFReader(registrationName='ExportedGLTF.gltf', FileName=filename)
renderView2 = GetActiveViewOrCreate('RenderView')
exportgltfDisplay = Show(exportgltf, renderView2, 'GeometryRepresentation')
exportgltfDisplay.SetRepresentationType('Surface')
# We don't truly load the texture but we do test
# the texture coordinates instead. Since we use
# the same color map, it should have the same
# display as the exported texture.
ColorBy(exportgltfDisplay, ('POINTS', 'TEXCOORD_0', 'Magnitude'))
exportgltfDisplay.RescaleTransferFunctionToDataRange(True, False)
exportgltfDisplay.SetScalarBarVisibility(renderView2, False)
renderView2.OrientationAxesVisibility = 0
renderView2.ResetCamera(False, 0.9)
renderView2.Update()

lut = GetColorTransferFunction('TEXCOORD_0')
lut.ApplyPreset('Cool to Warm', True)

# compare baseline with data read
# It is mostly useful to ensure the texture coordinates
# correctly exported.
try:
  baselineIndex = sys.argv.index('-B') + 1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")


baseline_file = os.path.join(baselinePath, "ExportGLTF.png")
from paraview.vtk.test import Testing
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView2.GetRenderWindow(), baseline_file)
Testing.interact()
