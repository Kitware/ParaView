ExternalData_Expand_Arguments(ParaViewData _
  # Baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/Clip.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/EnSight.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/ExtractBlock.png}")

vtk_add_test_cxx(vtkPVClientWebCxxTests tests
  NO_VALID NO_OUTPUT
  TestDataEncoder.cxx
  )
vtk_test_cxx_executable(vtkPVClientWebCxxTests tests)
