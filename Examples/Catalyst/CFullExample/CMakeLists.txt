if (USE_CATALYST)
  add_library(CFullExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CFullExampleAdaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel)
endif ()

add_executable(CFullExample
  FEDataStructures.c
  FEDataStructures.h
  FEDriver.c)
if (TARGET CFullExampleAdaptor)
  target_link_libraries(CFullExample
    PRIVATE
      CFullExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CFullExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CFullExampleTest COMMAND CFullExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
