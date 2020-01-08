if(PARAVIEW_USE_PYTHON AND VTK_PYTHON_FULL_THREADSAFE)
  vtk_module_test_executable(pqApplicationComponentsTestVtkPythonScopeGilEnsurer TestVtkPythonScopeGilEnsurer.cxx ${MOC_SRCS})
  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME pqApplicationComponentsTestVtkPythonScopeGilEnsurer
    COMMAND pqApplicationComponentsTestVtkPythonScopeGilEnsurer)
endif()
