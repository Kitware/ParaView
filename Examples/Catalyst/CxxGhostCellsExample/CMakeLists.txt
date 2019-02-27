if (NOT USE_CATALYST)
  message(STATUS
    "Skipping CxxHyperTreeGridExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_executable(CxxGhostCellsExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
target_link_libraries(CxxGhostCellsExample
  PRIVATE
    ParaView::PythonCatalyst
    VTK::CommonDataModel
    VTK::CommonExecutionModel
    VTK::mpi
    VTK::ParallelMPI)

if (BUILD_TESTING)
  add_test(NAME CxxGhostCellsExampleTest COMMAND CxxGhostCellsExample -s ${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/gridwriter.py)
endif()
