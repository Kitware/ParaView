# This plugin contains tools developed for the Stanford Linear Accelerator
# Center to make it easier to visualize finite element simulation results.

SET(PLUGIN_NAME SLACTools)
SET(PLUGIN_VERSION "0.0")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  )

SET(SM_XML
  SLACToolsSM.xml
  )

SET(SM_SRC
  vtkSLACPlaneGlyphs.cxx
  vtkSamplePlaneSource.cxx
  )

ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
  SERVER_MANAGER_XML ${SM_XML}
  SERVER_MANAGER_SOURCES ${SM_SRC}
  )
