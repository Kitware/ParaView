INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)

set(SOURCES
  vtkPrismCubeAxesRepresentation.cxx
  vtkPrismFilter.cxx
  vtkPrismRepresentation.cxx
  vtkPrismSESAMEReader.cxx
  vtkPrismSurfaceReader.cxx
  vtkPrismTableToPolyData.cxx
  vtkPrismView.cxx
  vtkSESAMEConversionFilter.cxx
  )

SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

ADD_PARAVIEW_PLUGIN(PrismServerPlugin "1.2"
  SERVER_MANAGER_SOURCES ${SOURCES}
  REQUIRED_ON_SERVER)
