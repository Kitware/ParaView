# win32 will require more work to correctly find the DLL's. See vtk/vtk#19531
if (TARGET ParaView::paraview AND PARAVIEW_USE_PYTHON AND NOT WIN32)
    set (TestVTKPython_USES_DIRECT_DATA ON)
    paraview_add_test_python(
      NO_DATA NO_VALID NO_RT
      TEST_SCRIPTS TestVTKPython.py
      )

    set_property(TEST "Python-TestVTKPython" APPEND
      PROPERTY
        ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/${python_destination}")
endif()
