INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Testing/Core/  
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )

set(vtk-module CGNSReader)
set(${vtk-module}_TEST_LABELS PARAVIEW)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT NO_DATA
  TestCGNSReader.cxx
  TestReadCGNSSolution.cxx
  )
  
ExternalData_add_test(ParaViewData
    NAME TestCGNSReader 
    COMMAND ${vtk-module}CxxTests TestCGNSReader
    -D DATA{../Data/Example_mixed.cgns}
       DATA{../Data/Example_nface_n.cgns}
    )

ExternalData_add_test(ParaViewData
    NAME TestReadCGNSSolution
    COMMAND ${vtk-module}CxxTests TestReadCGNSSolution
    -D DATA{../Data/channelBump_solution.cgns}
    )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
