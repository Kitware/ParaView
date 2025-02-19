include(ParaViewFindPythonModules)
find_python_module(numpy numpy_found)

if (numpy_found)
  paraview_add_test_python(
    NO_DATA NO_VALID NO_RT
    UsingPlugin.py)
endif()
