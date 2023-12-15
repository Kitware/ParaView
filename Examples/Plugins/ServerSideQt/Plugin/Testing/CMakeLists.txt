if (PARAVIEW_USE_PYTHON)
  set(TEST_ROOT ${CMAKE_BINARY_DIR}/Testing/Temporary)
  
  configure_file(
       ${CMAKE_CURRENT_SOURCE_DIR}/TestPython.py.in
       ${CMAKE_CURRENT_BINARY_DIR}/TestPython.py.in)
  file(GENERATE
       OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestPython.py
       INPUT ${CMAKE_CURRENT_BINARY_DIR}/TestPython.py.in)

  set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


  paraview_add_test_python(
    NO_RT NO_DATA NO_VALID
    TestPython.py)
endif()
