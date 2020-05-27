
from paraview.simple import *
from paraview import smtesting
import os.path
smtesting.ProcessCommandLineArguments()

Text(Text="Test Palette")
Show(FontSize=48)

Sphere()
Show(Representation="Wireframe", LineWidth=4)
Render()

# load print palette
LoadPalette("PrintBackground")
if not smtesting.DoRegressionTesting():
    raise RuntimeError("Test failed!")


statefilename = os.path.join(smtesting.TempDir, "ColorPaletteInStateFile.pvsm")
SaveState(statefilename)

# restore palette, not necessary but let's do it anyways.
LoadPalette("DefaultBackground")

# start a new session
ResetSession()

# load the state file
LoadState(statefilename)

# test that the palette has been loaded correctly.
if not smtesting.DoRegressionTesting():
    raise RuntimeError("Test failed!")
