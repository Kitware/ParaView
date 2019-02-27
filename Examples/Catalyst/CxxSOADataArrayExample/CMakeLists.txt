if (USE_CATALYST)
  add_library(CxxSOADataArrayExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxSOADataArrayExampleAdaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::ParallelMPI)
endif ()

add_executable(CxxSOADataArrayExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxSOADataArrayExampleAdaptor)
  target_link_libraries(CxxSOADataArrayExample
    PRIVATE
      CxxSOADataArrayExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxSOADataArrayExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxSOADataArrayExampleTest COMMAND CxxSOADataArrayExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
