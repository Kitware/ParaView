#### import the simple module from the paraview
from paraview.simple import *

# This filter is already being testing in the VTK side. We just look for its
# exposition in the ParaView GUI.
cylinder = Cylinder()
UpdatePipeline()
detector = OverlappingCellsDetector()
UpdatePipeline()
