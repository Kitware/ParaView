set(mpi_definitions
  MPICH_SKIP_MPICXX
  OMPI_SKIP_MPICXX)

if (USE_CATALYST)
  add_library(CxxVTKPipelineExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h
    FEDataStructures.cxx
    FEDataStructures.h
    vtkCPVTKPipeline.cxx
    vtkCPVTKPipeline.h)
  target_link_libraries(CxxVTKPipelineExampleAdaptor
    PRIVATE
      ParaView::Catalyst
      ParaView::RemotingServerManager
      ParaView::VTKExtensionsCore
      ParaView::VTKExtensionsFiltersGeneral
      ParaView::VTKExtensionsMisc
      VTK::CommonDataModel
      VTK::IOParallelXML
      VTK::ParallelCore
      MPI::MPI_C)
  target_compile_definitions(CxxVTKPipelineExampleAdaptor
    PUBLIC
      ${mpi_definitions})
  if (TARGET VTK::PythonUsed)
    target_link_libraries(CxxVTKPipelineExampleAdaptor
      INTERFACE
        VTK::PythonUsed)
  endif ()
endif ()

if (TARGET CxxVTKPipelineExampleAdaptor)
  add_executable(CxxVTKPipelineExample
    FEDriver.cxx)
  target_link_libraries(CxxVTKPipelineExample
    PRIVATE
      CxxVTKPipelineExampleAdaptor
      VTK::mpi
      VTK::CommonCore)
else ()
  add_executable(CxxVTKPipelineExample
    FEDriver.cxx
    FEDataStructures.cxx
    FEDataStructures.h)
  target_link_libraries(CxxVTKPipelineExample
    PRIVATE
      MPI::MPI_C
      VTK::CommonCore)
  target_compile_definitions(CxxVTKPipelineExample
    PUBLIC
      ${mpi_definitions})
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxVTKPipelineExampleTest COMMAND CxxVTKPipelineExample 10 output)
endif()
