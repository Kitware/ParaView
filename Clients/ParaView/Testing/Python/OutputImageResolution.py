# This test is for paraview/paraview#17843.
# This ensures that even when current sizes are not a perfect
# fraction of the requested size, we can generate an image of
# exact size.

from paraview.vtk.util.misc import vtkGetTempDir
from paraview.simple import *
from os.path import join

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [300, 259]
renderView1.Background = [0.32, 0.34, 0.43]

AssignViewToLayout(renderView1)

layout1 = GetLayout()
layout1.SeparatorWidth = 2
layout1.SplitVertical(0, 0.5)

# Create a new 'Render View'
renderView2 = CreateView('RenderView')
renderView2.ViewSize = [300, 258]
renderView2.Background = [0.32, 0.34, 0.43]
layout1.AssignView(2, renderView2)

RenderAllViews()

a = [0, 0, 0, 0]
layout1.GetLayoutExtent(a)
size = (a[1] - a[0] + 1, a[3] - a[2] + 1)

# Ensure that the current size has odd height
assert size[1] % 2 != 0
assert size[1] == (renderView1.ViewSize[1] + renderView2.ViewSize[1] + layout1.SeparatorWidth)

def SaveAndCheckSize(filename, layout, resolution):
    SaveScreenshot(filename, layout, SaveAllViews=1, ImageResolution=resolution)

    from paraview.vtk.vtkIOImage import vtkPNGReader
    reader = vtkPNGReader()
    reader.SetFileName(filename)
    reader.Update()
    assert (reader.GetOutput().GetDimensions()[0:2] == resolution)

SaveAndCheckSize(join(vtkGetTempDir(), "OutputImageResolution300.png"), layout1, (300, 300))
SaveAndCheckSize(join(vtkGetTempDir(), "OutputImageResolution313.png"), layout1, (300, 313))
