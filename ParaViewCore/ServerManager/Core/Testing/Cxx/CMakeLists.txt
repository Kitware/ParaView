paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID
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

if ((DEFINED CMAKE_CXX_STANDARD) AND (NOT CMAKE_CXX_STANDARD EQUAL 98) AND CMAKE_CXX_STANDARD_REQUIRED)
set(cxx11_tests TestAdjustRange.cxx)
  paraview_add_test_cxx(${vtk-module}CxxTests
    NO_DATA NO_VALID
    ${cxx11_tests})
  list(APPEND tests ${cxx11_tests})
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
