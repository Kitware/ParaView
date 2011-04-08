IF (NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (NOT ParaView_BINARY_DIR)

ADD_PARAVIEW_PLUGIN(Representation "1.0"
  SERVER_MANAGER_XML Representation.xml
  SERVER_MANAGER_SOURCES
    vtkMySpecialPolyDataMapper.cxx
    vtkMySpecialRepresentation.cxx)
