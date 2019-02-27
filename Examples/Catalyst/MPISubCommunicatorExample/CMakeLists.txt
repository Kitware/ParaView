if (NOT USE_CATALYST)
  message(STATUS
    "Skipping MPISubCommunicatorExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_executable(MPISubCommunicatorExample
  FEAdaptor.cxx
  FEAdaptor.h
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
target_link_libraries(MPISubCommunicatorExample
  PRIVATE
    ParaView::PythonCatalyst
    VTK::CommonDataModel
    VTK::ParallelMPI)

if (BUILD_TESTING)
  add_test(NAME MPISubCommunicatorExampleTest COMMAND MPISubCommunicatorExample
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
