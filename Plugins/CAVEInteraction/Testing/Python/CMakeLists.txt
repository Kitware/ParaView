# supply the test-name prefix vtk_module_build normally provides
set(_vtk_build_test "CAVEInteraction")

# Basic test of custom python interactor style proxy functionality
paraview_add_test_python(
  TestPythonInteractorStyleSelf.py,NO_DATA,NO_VALID,NO_OUTPUT)
