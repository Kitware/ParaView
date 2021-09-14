# Tests BUG #20133

from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

LoadState(smtesting.DataDir + "/Testing/Data/StateWithHiddenRepresentations.pvsm")

wavelet = FindSource("Wavelet1")
contour = FindSource("Contour1")
view = GetRenderView()

# Render view with everything hidden.
view.StillRender()

# All data informations should be empty
assert  wavelet.GetDataInformation().GetNumberOfPoints() == 0
assert contour.GetDataInformation().GetNumberOfPoints() == 0

# Show contour
display = GetRepresentation(contour, view)
display.Visibility = 1

# Render
view.StillRender()

# Now, all data information should be non-empty
assert  wavelet.GetDataInformation().GetNumberOfPoints() > 0
assert contour.GetDataInformation().GetNumberOfPoints() > 0
