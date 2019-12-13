vtk_module_test_data(
  Data/can.ex2)

add_subdirectory(Cxx)

if (PARAVIEW_USE_PYTHON)
  add_subdirectory(Python)
endif ()
