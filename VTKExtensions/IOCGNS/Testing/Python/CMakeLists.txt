if (PARAVIEW_USE_PYTHON)
  if (TARGET VTK::IOCGNSReader)
    ExternalData_Expand_Arguments(ParaViewData _
      # data files
      "DATA{${paraview_test_data_directory_input}/Data/channelBump_solution.cgns}"
    )

    set(PY_TESTS
      ChooseArrays.py,NO_VALID,NO_OUTPUT
      )

    paraview_add_test_python(
      ${PY_TESTS}
      )

  endif()
  if (TARGET VTK::IOExodus)
    ExternalData_Expand_Arguments(ParaViewData _
      # data files
      "DATA{${paraview_test_data_directory_input}/Data/can.ex2}"
    )

    set(PY_TESTS
      WriteAllTimesteps.py,NO_VALID,NO_OUTPUT
      )

    paraview_add_test_python(
      ${PY_TESTS}
      )

  endif()
endif()
