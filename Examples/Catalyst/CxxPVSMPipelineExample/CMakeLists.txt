set(mpi_definitions
  MPICH_SKIP_MPICXX
  OMPI_SKIP_MPICXX)

# Note: this actually only requires ParaView::Catalyst.
if (USE_CATALYST)
  add_library(CxxPVSMPipelineExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h
    FEDataStructures.cxx
    FEDataStructures.h
    vtkCPPVSMPipeline.cxx
    vtkCPPVSMPipeline.h)
  target_link_libraries(CxxPVSMPipelineExampleAdaptor
    PRIVATE
      ParaView::Catalyst
      ParaView::RemotingServerManager
      VTK::CommonDataModel
      MPI::MPI_C)
  target_compile_definitions(CxxPVSMPipelineExampleAdaptor
    PUBLIC
      ${mpi_definitions})
  if (TARGET VTK::PythonUsed)
    target_link_libraries(CxxPVSMPipelineExampleAdaptor
      INTERFACE
        VTK::PythonUsed)
  endif ()
endif ()

if (TARGET CxxPVSMPipelineExampleAdaptor)
  add_executable(CxxPVSMPipelineExample
    FEDriver.cxx)
  target_link_libraries(CxxPVSMPipelineExample
    PRIVATE
      CxxPVSMPipelineExampleAdaptor
      VTK::mpi)
else ()
  add_executable(CxxPVSMPipelineExample
    FEDriver.cxx
    FEDataStructures.cxx
    FEDataStructures.h)
  target_link_libraries(CxxPVSMPipelineExample
    PRIVATE
      MPI::MPI_C)
  target_compile_definitions(CxxPVSMPipelineExample
      PUBLIC
        ${mpi_definitions})
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxPVSMPipelineExampleTest COMMAND CxxPVSMPipelineExample 5 output)
endif()
