# Tests that an image extractor can save all views in a layout, created using
# `CreateLayout`, as a single image when running in situ.
# script-version: 2.0

from paraview.simple import *
from paraview import catalyst

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

# use distinct background colors to identify each view in the extracts.
renderView1 = CreateView('RenderView')
renderView1.OrientationAxesVisibility = 0
renderView1.UseColorPaletteForBackground = 0
renderView1.Background = [1.0, 0.0, 0.0]

renderView2 = CreateView('RenderView')
renderView2.OrientationAxesVisibility = 0
renderView2.UseColorPaletteForBackground = 0
renderView2.Background = [0.0, 0.0, 1.0]

# ----------------------------------------------------------------
# setup view layouts
# ----------------------------------------------------------------

layout1 = CreateLayout(name='Layout #1')
layout1.SplitHorizontal(0, 0.5)
layout1.AssignView(1, renderView1)
layout1.AssignView(2, renderView2)
layout1.SetSize(400, 200)

# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

wavelet1 = Wavelet(registrationName='input')
slice1 = Slice(registrationName='Slice1', Input=wavelet1)
slice1.SliceType.Normal = [0.0, 0.0, 1.0]

# ----------------------------------------------------------------
# setup the visualization
# ----------------------------------------------------------------

wavelet1Display = Show(wavelet1, renderView1)
wavelet1Display.Representation = 'Outline'

slice1Display = Show(slice1, renderView2)

# ----------------------------------------------------------------
# setup extractors
# ----------------------------------------------------------------

pNG1 = CreateExtractor('PNG', renderView1, registrationName='PNG1')
pNG1.Writer.FileName = 'layout_{timestep:06d}.png'
pNG1.Writer.ResetDisplay = 1
pNG1.Writer.SaveAllViews = 1
pNG1.Writer.ImageResolution = [400, 200]
pNG1.Writer.SeparatorWidth = 4
pNG1.Writer.SeparatorColor = [0.0, 1.0, 0.0]

# ------------------------------------------------------------------------------
# Catalyst options
options = catalyst.Options()
options.GlobalTrigger = 'TimeStep'


# ------------------------------------------------------------------------------
def validate_image(fname):
    from paraview.vtk.vtkIOImage import vtkPNGReader

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


def catalyst_finalize():
    from paraview.modules.vtkRemotingCore import vtkProcessModule
    if vtkProcessModule.GetProcessModule().GetPartitionId() > 0:
        # images are only written on the root node.
        return

    import glob, os, os.path
    outdir = os.environ.get("PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY",
                            options.ExtractsOutputDirectory)
    fnames = sorted(glob.glob(os.path.join(outdir, "layout_*.png")))
    if not fnames:
        raise RuntimeError("Test failed! No extracts generated under '%s'" % outdir)

    for fname in fnames:
        validate_image(fname)
    print("validated", len(fnames), "extracts")
    print("All ok")
