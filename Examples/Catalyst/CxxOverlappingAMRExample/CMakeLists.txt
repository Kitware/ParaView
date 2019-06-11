if (NOT USE_CATALYST)
  message(STATUS
    "Skipping CxxOverlappingAMRExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_library(CxxOverlappingAMRExampleAdaptor
  FEAdaptor.cxx
  FEAdaptor.h)
target_link_libraries(CxxOverlappingAMRExampleAdaptor
  INTERFACE
    VTK::PythonUsed
  PRIVATE
    ParaView::PythonCatalyst
    VTK::CommonDataModel
    VTK::ParallelCore)

add_executable(CxxOverlappingAMRExample
  FEDriver.cxx)
target_link_libraries(CxxOverlappingAMRExample
  PRIVATE
    CxxOverlappingAMRExampleAdaptor)

if (BUILD_TESTING)
  add_test(NAME CxxOverlappingAMRExampleTest COMMAND CxxOverlappingAMRExample
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
