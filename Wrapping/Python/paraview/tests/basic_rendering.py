r"""verify basic surface rendering"""

from . import internals
from ..simple import *
from .. import print_info as log

def Execute(opts):
    log("initialize pipeline")
    view = CreateView('RenderView')
    view.ViewSize = [400, 400]

    layout = CreateLayout()
    layout.AssignView(0, view)
    layout.SetSize(400, 400)

    Sphere(PhiResolution=1000, ThetaResolution=1000)
    Elevation(LowPoint=[-0.5, -0.5, 0.], HighPoint=[0.5, 0.5, 0])
    disp = Show()
    disp.SetRepresentationType("Surface")

    # setup LUT
    elevationLUT = GetColorTransferFunction('Elevation')
    elevationLUT.RescaleTransferFunction(0.0, 1.0)
    ColorBy(disp, ('POINTS', 'Elevation'))

    # reset camera
    view.ResetCamera(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    log("begin render")
    Render()
    log("end render")
    if opts.interactive:
        Interact()
    if opts.output:
        log("save test image")
        SaveScreenshot(opts.output, ImageResolution=[400, 400])
        if opts.baseline:
            log("compare baseline image")
            internals.compare(opts.output, opts.baseline)
    elif opts.baseline:
        raise RuntimeError("baseline (-v) specified without output (-o)")


def main(args=None):
    import argparse
    parser = argparse.ArgumentParser(description='Test basic surface rendering.')
    parser.add_argument("-i", "--interactive", help="enable interaction", action="store_true")
    parser.add_argument("-o", "--output", help="output image file", type=str)
    parser.add_argument("-v", "--baseline", help="baseline image (for comparison)", type=str)
    Execute(parser.parse_args(args))


if __name__ == "__main__":
    main()
