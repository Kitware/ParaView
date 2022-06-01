from paraview.simple import *

# Check that `None` is a supported value when setting the material library
renderView1 = CreateView('RenderView')
renderView1.OSPRayMaterialLibrary = None
