set(mpi_definitions
  MPICH_SKIP_MPICXX
  OMPI_SKIP_MPICXX)

if (USE_CATALYST)
  add_library(CachingExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h
    FEDataStructures.cxx
    FEDataStructures.h)
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
      VTK::IOXML
      MPI::MPI_C)
  target_compile_definitions(CachingExampleAdaptor
    PUBLIC
      ${mpi_definitions})
endif ()

if (TARGET CachingExampleAdaptor)
  add_executable(CachingExample
    FEDriver.cxx)
  target_link_libraries(CachingExample
    PRIVATE
      CachingExampleAdaptor
      VTK::mpi)
else ()
  find_package(ParaView REQUIRED)
  add_executable(CachingExample
    FEDriver.cxx
    FEDataStructures.cxx
    FEDataStructures.h)
  target_link_libraries(CachingExample
    PRIVATE
      VTK::CommonSystem
      VTK::CommonCore
      MPI::MPI_C)
  target_compile_definitions(CachingExample
    PUBLIC
      ${mpi_definitions})
endif ()

if (BUILD_TESTING)
  add_test(NAME CachingExampleTest COMMAND CachingExample ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/exPostFactoMovie.py)
endif()
