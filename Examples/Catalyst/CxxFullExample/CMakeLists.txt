if (USE_CATALYST)
  add_library(CxxFullExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxFullExampleAdaptor
    INTERFACE
      VTK::PythonUsed
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel)
endif ()

add_executable(CxxFullExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxFullExampleAdaptor)
  target_link_libraries(CxxFullExample
    PRIVATE
      CxxFullExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxFullExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxFullExampleTest COMMAND CxxFullExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
