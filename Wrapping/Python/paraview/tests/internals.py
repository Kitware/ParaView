r"""private module with utilities to be used by modules in this package alone"""

def compare(test_image, baseline_image):
    from vtkmodules.vtkTestingRendering import vtkTesting
    testing = vtkTesting()
    testing.AddArgument("-V")
    testing.AddArgument(baseline_image)
    if testing.RegressionTest(test_image, 10) == vtkTesting.FAILED:
        raise RuntimeError("Regression test failed!")
    return True
