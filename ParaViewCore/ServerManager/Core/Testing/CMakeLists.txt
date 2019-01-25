vtk_module_test_data(
  Data/can.ex2)

add_subdirectory(Cxx)

if (PARAVIEW_ENABLE_PYTHON)
  add_subdirectory(Python)
endif ()
