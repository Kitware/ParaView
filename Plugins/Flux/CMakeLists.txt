# This plugin contains a set of filters that helps analysis of flux and
# circulation fields.  Flux fields are defined on 2D cells and describe
# flows through the area of the cell.  Circulation fields are defined on 1D
# cells and describes flows in the direction of the cell.

SET(PLUGIN_NAME Flux)
SET(PLUGIN_VERSION "0.0")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  )

SET(SM_XML
  Flux.xml
  )

SET(SM_SRC
  vtkCirculationFeatures.cxx
  vtkFluxVectors.cxx
  )

ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
  SERVER_MANAGER_XML ${SM_XML}
  SERVER_MANAGER_SOURCES ${SM_SRC}
  )
