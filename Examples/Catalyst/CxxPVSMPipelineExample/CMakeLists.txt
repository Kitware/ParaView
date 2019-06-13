# Note: this actually only requires ParaView::Catalyst.
if (USE_CATALYST)
  add_library(CxxPVSMPipelineExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h
    vtkCPPVSMPipeline.cxx
    vtkCPPVSMPipeline.h)
  target_link_libraries(CxxPVSMPipelineExampleAdaptor
    PRIVATE
      ParaView::Catalyst
      ParaView::VTKExtensionsDefault
      ParaView::ServerManagerCore)
  if (TARGET VTK::PythonUsed)
    target_link_libraries(CxxPVSMPipelineExampleAdaptor
      INTERFACE
        VTK::PythonUsed)
  endif ()
endif ()

add_executable(CxxPVSMPipelineExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxPVSMPipelineExampleAdaptor)
  target_link_libraries(CxxPVSMPipelineExample
    PRIVATE
      CxxPVSMPipelineExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxPVSMPipelineExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (BUILD_TESTING)
  add_test(NAME CxxPVSMPipelineExampleTest COMMAND CxxPVSMPipelineExample 5 output)
endif()
