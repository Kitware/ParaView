if (USE_CATALYST)
  add_library(CxxMultiChannelInputExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxMultiChannelInputExampleAdaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::ParallelMPI)
endif ()

add_executable(CxxMultiChannelInputExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxMultiChannelInputExampleAdaptor)
  target_link_libraries(CxxMultiChannelInputExample
    PRIVATE
      CxxMultiChannelInputExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxMultiChannelInputExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxMultiChannelInputExampleTest COMMAND CxxMultiChannelInputExample ${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/allinputsgridwriter.py)
endif()
