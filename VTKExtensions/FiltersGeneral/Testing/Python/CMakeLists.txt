if (PARAVIEW_USE_PYTHON)
  ExternalData_Expand_Arguments(ParaViewData _
    # data files
    "DATA{${paraview_test_data_directory_input}/Data/channelBump_solution.cgns}"
  )
  
  set(PY_TESTS
    ConvertPolyhedra.py,NO_VALID,NO_OUTPUT
    )

  paraview_add_test_python(
    ${PY_TESTS}
    )

endif(PARAVIEW_USE_PYTHON)
