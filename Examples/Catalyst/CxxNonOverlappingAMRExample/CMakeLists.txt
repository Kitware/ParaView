if (NOT USE_CATALYST)
  message(STATUS
    "Skipping CxxNonOverlappingAMRExample because `USE_CATALYST` is not set.")
  return ()
endif ()

add_library(CxxNonOverlappingAMRExampleAdaptor
  FEAdaptor.cxx
  FEAdaptor.h)
target_link_libraries(CxxNonOverlappingAMRExampleAdaptor
  PRIVATE
    ParaView::PythonCatalyst
    VTK::CommonDataModel)

add_executable(CxxNonOverlappingAMRExample
  FEDriver.cxx)
target_link_libraries(CxxNonOverlappingAMRExample
  PRIVATE
    CxxNonOverlappingAMRExampleAdaptor)

if (BUILD_TESTING)
  add_test(NAME CxxNonOverlappingAMRExampleTest COMMAND CxxNonOverlappingAMRExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
