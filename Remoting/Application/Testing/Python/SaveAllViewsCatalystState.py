# Tests image extractors that save all views in a layout as a single image:
# generating the extracts directly, exporting a Catalyst state (which must
# include the layout and all the views in it) and running that state.

from paraview.simple import *
from paraview import catalyst, smtesting
from paraview.detail.catalyst_export import save_catalyst_state
from paraview.vtk.vtkIOImage import vtkPNGReader

import os.path
import re

smtesting.ProcessCommandLineArguments()
tempdir = smtesting.GetUniqueTempDirectory("SaveAllViewsCatalystState-")
print("Generating output files in `%s`" % tempdir)


def setup_visualization():
    # use distinct background colors to identify each view in the extracts.
    renderView1 = CreateView('RenderView')
    renderView1.OrientationAxesVisibility = 0
    renderView1.UseColorPaletteForBackground = 0
    renderView1.Background = [1.0, 0.0, 0.0]

    renderView2 = CreateView('RenderView')
    renderView2.OrientationAxesVisibility = 0
    renderView2.UseColorPaletteForBackground = 0
    renderView2.Background = [0.0, 0.0, 1.0]

    layout1 = CreateLayout(name='Layout #1')
    layout1.SplitHorizontal(0, 0.5)
    layout1.AssignView(1, renderView1)
    layout1.AssignView(2, renderView2)
    layout1.SetSize(400, 200)

    wavelet1 = Wavelet(registrationName='Wavelet1')
    wavelet1Display = Show(wavelet1, renderView1)
    wavelet1Display.Representation = 'Outline'

    # the slice is only shown in the view without an extractor.
    slice1 = Slice(registrationName='Slice1', Input=wavelet1)
    slice1.SliceType.Normal = [0.0, 0.0, 1.0]
    Show(slice1, renderView2)

    pNG1 = CreateExtractor('PNG', renderView1, registrationName='PNG1')
    pNG1.Writer.FileName = 'layout_{timestep:06d}.png'
    pNG1.Writer.ResetDisplay = 1
    pNG1.Writer.SaveAllViews = 1
    pNG1.Writer.ImageResolution = [400, 200]
    pNG1.Writer.SeparatorWidth = 4
    pNG1.Writer.SeparatorColor = [0.0, 1.0, 0.0]


def validate_image(fname):
    if not os.path.exists(fname):
        raise RuntimeError("Test failed! Missing extract '%s'" % fname)

    reader = vtkPNGReader()
    reader.SetFileName(fname)
    reader.Update()
    image = reader.GetOutput()

    dims = image.GetDimensions()
    if dims[0] != 400 or dims[1] != 200:
        raise RuntimeError("Test failed! Unexpected image size %s for '%s'" % (dims, fname))

    scalars = image.GetPointData().GetScalars()

    def get_color(x, y):
        idx = y * dims[0] + x
        return [int(scalars.GetComponent(idx, c)) for c in range(3)]

    def is_color(color, expected):
        return all(abs(a - b) <= 5 for a, b in zip(color, expected))

    def check_color(x, y, expected):
        color = get_color(x, y)
        if not is_color(color, expected):
            raise RuntimeError("Test failed! Expected color %s at (%d, %d) in '%s', got %s" %
                               (expected, x, y, fname, color))

    # corners of the left (renderView1) and right (renderView2) halves.
    check_color(2, 2, [255, 0, 0])
    check_color(2, 197, [255, 0, 0])
    check_color(397, 2, [0, 0, 255])
    check_color(397, 197, [0, 0, 255])

    # separator between the two views.
    check_color(200, 2, [0, 255, 0])
    width = len([x for x in range(dims[0]) if is_color(get_color(x, 2), [0, 255, 0])])
    if width != 4:
        raise RuntimeError("Test failed! Expected separator width of 4, got %d in '%s'" %
                           (width, fname))


setup_visualization()

# 1. generate extracts directly.
SaveExtracts(ExtractsOutputDirectory=os.path.join(tempdir, "ImmediateExtracts"))
validate_image(os.path.join(tempdir, "ImmediateExtracts", "layout_000000.png"))

# 2. export Catalyst state.
options = catalyst.Options()
options.ExtractsOutputDirectory = os.path.join(tempdir, "BatchExtracts")
script = os.path.join(tempdir, "catalyst_state.py")
save_catalyst_state(script, options)

with open(script, "r") as f:
    state = f.read()
print(state)

# the layout and all views in it must be saved, including pipelines only shown
# in views without an extractor.
for expected in ["CreateLayout(name='Layout #1')",
                 "AssignView(1, renderView1)",
                 "AssignView(2, renderView2)",
                 "Slice(registrationName='Slice1'",
                 "Show(slice1, renderView2",
                 "CreateExtractor('PNG', renderView1"]:
    if expected not in state:
        raise RuntimeError("Test failed! Missing '%s' in Catalyst state." % expected)
if not re.search(r"SaveAllViews\s*=\s*1", state):
    raise RuntimeError("Test failed! Missing 'SaveAllViews' in Catalyst state.")

# 3. run the Catalyst state in a new session, as done in batch mode.
ResetSession()
exec(compile(state, script, "exec"), {"__name__": "__main__"})
validate_image(os.path.join(tempdir, "BatchExtracts", "layout_000000.png"))
print("All ok")
