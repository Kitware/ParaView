if (USE_CATALYST)
  add_library(CachingExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CachingExampleAdaptor
    INTERFACE
      VTK::PythonUsed
    PUBLIC
      ParaView::PythonCatalyst
      ParaView::RemotingServerManager
      VTK::CommonCore
      VTK::CommonDataModel
      VTK::CommonSystem
      VTK::FiltersGeneral
      VTK::FiltersHybrid
      VTK::IOXML)
endif ()

add_executable(CachingExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CachingExampleAdaptor)
  target_link_libraries(CachingExample
    PRIVATE
      VTK::CommonCore
      CachingExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CachingExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CachingExampleTest COMMAND CachingExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/exPostFactoMovie.py)
endif()
