if (USE_CATALYST)
  add_library(CxxMultiPieceExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxMultiPieceExampleAdaptor
    INTERFACE
      VTK::PythonUsed
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::mpi
      VTK::ParallelCore)
endif ()

add_executable(CxxMultiPieceExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxMultiPieceExampleAdaptor)
  target_link_libraries(CxxMultiPieceExample
    PRIVATE
      CxxMultiPieceExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxMultiPieceExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxMultiPieceExampleTest COMMAND CxxMultiPieceExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
