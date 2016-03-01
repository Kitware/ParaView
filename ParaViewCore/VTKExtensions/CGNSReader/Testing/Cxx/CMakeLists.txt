INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Testing/Core/  
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )

set(vtk-module VTKExtensions)
set(${vtk-module}_TEST_LABELS PARAVIEW)

ExternalData_add_test(ParaViewData
    NAME TestCGNSReader 
    COMMAND vtkPVVTKExtensionsCGNSReaderTests TestCGNSReader
    DATA{../Data/Example_mixed.cgns}
    DATA{../Data/Example_nface_n.cgns}
    )

ExternalData_add_test(ParaViewData
    NAME TestReadCGNSSolution
    COMMAND vtkPVVTKExtensionsCGNSReaderTests TestReadCGNSSolution
    DATA{../Data/channelBump_solution.cgns}
    )

vtk_add_test_cxx(${vtk-modules}vtkPVVTKExtensionsCGNSReaderTests tests
  NO_VALID NO_OUTPUT
  TestCGNSReader.cxx
  TestReadCGNSSolution.cxx
  )
  
vtk_test_cxx_executable(${vtk-modules}vtkPVVTKExtensionsCGNSReaderTests tests)
