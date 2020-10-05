from paraview.simple import *


def Compare(testimage, baseimage):
    from vtkmodules.vtkTestingRendering import vtkTesting
    testing = vtkTesting()
    testing.AddArgument("-V")
    testing.AddArgument(baseimage)
    if testing.RegressionTest(testimage, 10) == vtkTesting.FAILED:
        raise RuntimeError("Regression test failed!")

def Execute(opts):
    CreateView('RenderViewWithEDL')
    Sphere()
    Show()
    Render()
    if opts.interactive:
        Interact()
    if opts.output:
        SaveScreenshot(opts.output)
        if opts.baseline:
            Compare(opts.output, opts.baseline)
    elif opts.baseline:
        raise RuntimeError("baseline (-v) specified without output (-o)")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Test Eye-Dome Lighthing.')
    parser.add_argument("-i", "--interactive", help="enable interaction", action="store_true")
    parser.add_argument("-o", "--output", help="output image file", type=str)
    parser.add_argument("-v", "--baseline", help="baseline image (for comparison)", type=str)
    args = parser.parse_args()
    Execute(args)
