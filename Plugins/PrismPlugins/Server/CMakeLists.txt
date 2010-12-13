INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)



SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

ADD_PARAVIEW_PLUGIN(PrismServerPlugin "1.1"
  SERVER_MANAGER_SOURCES vtkPrismFilter.cxx vtkPrismSurfaceReader.cxx vtkPrismCubeAxesActor.cxx vtkPrismSESAMEReader.cxx vtkPrismCubeAxesRepresentation.cxx vtkPrismRepresentation.cxx vtkPrismView.cxx
  REQUIRED_ON_SERVER)
