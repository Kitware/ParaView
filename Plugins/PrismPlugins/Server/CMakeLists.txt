INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)




ADD_PARAVIEW_PLUGIN(PrismServerPlugin "1.1"
  SERVER_MANAGER_SOURCES vtkPrismFilter.cxx vtkPrismSurfaceReader.cxx vtkPrismCubeAxesActor.cxx vtkPrismSESAMEReader.cxx
  REQUIRED_ON_SERVER)
