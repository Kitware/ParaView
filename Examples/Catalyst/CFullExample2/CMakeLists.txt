if (USE_CATALYST)
  add_library(CFullExample2Adaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CFullExample2Adaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel)
endif ()

add_executable(CFullExample2
  FEDataStructures.c
  FEDataStructures.h
  FEDriver.c)
if (TARGET CFullExample2Adaptor)
  target_link_libraries(CFullExample2
    PRIVATE
      CFullExample2Adaptor
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::mpi)
else ()
  target_link_libraries(CFullExample2
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CFullExample2Test COMMAND CFullExample2 ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
