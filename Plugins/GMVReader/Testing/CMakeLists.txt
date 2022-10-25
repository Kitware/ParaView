set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY ${paraview_test_data_directory_output})

if (PARAVIEW_USE_PYTHON)
  vtk_module_test_data(
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/GMV/one_vertex.gmv
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/GMV/two_vertex.gmv
    )
  add_subdirectory(Python)
endif()

add_subdirectory(XML)
