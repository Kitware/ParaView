set(module_tests
  GmshIOTest.xml)

if (BUILD_SHARED_LIBS)
  set(_paraview_add_tests_default_test_data_target ParaViewData)

  ExternalData_Expand_Arguments(ParaViewData _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Gmsh/hemisphere.msh}"

    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestA.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestB.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestC.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestD.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestE.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/GmshIOTestF.png}"
  )

  paraview_add_client_tests(
    LOAD_PLUGIN   "GmshIO"
    BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS  ${module_tests})

  paraview_add_client_server_tests(
    LOAD_PLUGIN   "GmshIO"
    BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS  ${module_tests})

  paraview_add_client_server_render_tests(
    LOAD_PLUGIN   "GmshIO"
    BASELINE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS  ${module_tests})
endif ()
