# This plugin contains a set of filters that helps analysis of flux and
# circulation fields.  Flux fields are defined on 2D cells and describe
# flows through the area of the cell.  Circulation fields are defined on 1D
# cells and describes flows in the direction of the cell.

include_directories(${VTK_INCLUDE_DIRS})

SET(PLUGIN_NAME Moments)
SET(PLUGIN_VERSION "0.0")

SET(SM_XML
  Moments.xml
  )

SET(SM_SRC
  vtkMomentGlyphs.cxx
  vtkMomentVectors.cxx
  )

ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML ${SM_XML}
  SERVER_MANAGER_SOURCES ${SM_SRC}
  )
