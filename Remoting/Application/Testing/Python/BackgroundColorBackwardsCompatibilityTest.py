# state file generated using paraview version 5.9.0-RC4
import paraview
from paraview.simple import *

renderView1 = CreateView('RenderView')
renderView1.ViewSize = [844, 539]
renderView1.Background2 = [0.5, 0, 0]

try:
    renderView1.UseGradientBackground = 1
except paraview.NotSupportedException:
    pass
else:
    raise RuntimeError("NotSupportedException not thrown")

try:
    renderView1.UseTexturedBackground = 1
except paraview.NotSupportedException:
    pass
else:
    raise RuntimeError("NotSupportedException not thrown")

try:
    renderView1.UseSkyboxBackground = 1
except paraview.NotSupportedException:
    pass
else:
    raise RuntimeError("NotSupportedException not thrown")


# Now force older version and try the same thing again
paraview.compatibility.major = 5
paraview.compatibility.minor = 9

renderView1.UseGradientBackground = 1
assert(renderView1.BackgroundColorMode == "Gradient")
assert(renderView1.UseGradientBackground == 1)

renderView1.UseTexturedBackground = 1
assert(renderView1.BackgroundColorMode == "Texture")
assert(renderView1.UseTexturedBackground == 1)

renderView1.UseTexturedBackground = 0
assert(renderView1.BackgroundColorMode == "Single Color")
assert(renderView1.UseTexturedBackground == 0)

renderView1.UseSkyboxBackground = 1
assert(renderView1.BackgroundColorMode == "Skybox")
assert(renderView1.UseSkyboxBackground == 1)

renderView1.UseSkyboxBackground = 0
assert(renderView1.BackgroundColorMode == "Single Color")
assert(renderView1.UseSkyboxBackground == 0)
