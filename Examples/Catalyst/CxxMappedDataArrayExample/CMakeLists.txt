if (USE_CATALYST)
  add_library(CxxMappedDataArrayExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h
    vtkCPMappedVectorArrayTemplate.h
    vtkCPMappedVectorArrayTemplate.txx)
  target_link_libraries(CxxMappedDataArrayExampleAdaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::ParallelMPI)
endif ()

add_executable(CxxMappedDataArrayExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxMappedDataArrayExampleAdaptor)
  target_link_libraries(CxxMappedDataArrayExample
    PRIVATE
      CxxMappedDataArrayExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxMappedDataArrayExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxMappedDataArrayExampleTest COMMAND CxxMappedDataArrayExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
