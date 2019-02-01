if (NOT USE_CATALYST)
  message(STATUS
    "Skipping CxxHyperTreeGridExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_executable(CxxHyperTreeGridExample
  FEAdaptor.cxx
  FEAdaptor.h
  FEDriver.cxx)
target_link_libraries(CxxHyperTreeGridExample
  PRIVATE
    ParaView::PythonCatalyst
    VTK::ParallelMPI
    VTK::mpi)

if (BUILD_TESTING)
  add_test(NAME CxxHyperTreeGridExampleTest COMMAND CxxHyperTreeGridExample ${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/gridwriter.py)
endif()
