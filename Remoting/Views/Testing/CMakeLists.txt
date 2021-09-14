add_subdirectory(Cxx)

if (PARAVIEW_USE_PYTHON)
  vtk_module_test_data(Data/x_ray_copy_1.json)
  add_subdirectory(Python)
endif ()
