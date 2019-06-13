if (USE_CATALYST)
  add_library(CxxImageDataExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxImageDataExampleAdaptor
    INTERFACE
      VTK::PythonUsed
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel)
endif ()

add_executable(CxxImageDataExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxImageDataExampleAdaptor)
  target_link_libraries(CxxImageDataExample
    PRIVATE
      CxxImageDataExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxImageDataExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxImageDataExampleTest COMMAND CxxImageDataExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
