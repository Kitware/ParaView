if (PARAVIEW_USE_PYTHON AND TARGET ParaView::pvpython)
  # Configure the test script - first substitute CMake variables, then generator expressions
  set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestMultiOutputPortReader.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TestMultiOutputPortReader.py.configured"
    @ONLY)

  file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/TestMultiOutputPortReader.py"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/TestMultiOutputPortReader.py.configured")

  set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  set(_vtk_build_TEST_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  paraview_add_test_python(
    NO_RT DIRECT_DATA
    TestMultiOutputPortReader.py)
endif()
