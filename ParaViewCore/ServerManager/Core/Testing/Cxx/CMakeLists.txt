paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID
  TestAdjustRange.cxx
  TestSelfGeneratingSourceProxy.cxx
  TestSessionProxyManager.cxx
  TestSettings.cxx
  TestRecreateVTKObjects.cxx
  )

if(NOT PARAVIEW_BUILD_QT_GUI)
  paraview_test_load_data("" can.ex2)
endif()
paraview_add_test_cxx(${vtk-module}CxxTests tmp_tests
  NO_VALID
  TestParaViewPipelineController.cxx
  )
list(APPEND tests
  ${tmp_tests})

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
