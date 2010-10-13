INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
  
# Qt interface
SET( MOC_SRCS )
SET( UI_SRCS )

IF (PARAVIEW_BUILD_QT_GUI)

  INCLUDE( ${QT_USE_FILE} )

  QT4_WRAP_CPP( MOC_SRCS 
    pqSurfaceLICDisplayPanelDecorator.h
    )

  QT4_WRAP_UI( UI_SRCS 
    pqSurfaceLICDisplayPanelDecorator.ui
    )
    
  INCLUDE_DIRECTORIES(
    ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${QtWidgets_BINARY_DIR}
    ${QtWidgets_SOURCE_DIR}
    
    ${pqComponents_BINARY_DIR}
    ${pqComponents_SOURCE_DIR}
    ${pqCore_BINARY_DIR}
    ${pqCore_SOURCE_DIR}
    )
    
  SET( SURFACE_LIC_SOURCES
    pqSurfaceLICDisplayPanelDecorator.cxx
    )

  # server manager
  SET( SM_SOURCES
    vtkSurfaceLICRepresentation.cxx
    )

  SET( SM_XMLS
    vtkSurfaceLICRendering.xml
    vtkSurfaceLICExtension.xml
    )
    
  # This is the macro to add a display panel decorator.
  # It needs the class name, and the panel types we are decorating. It fills up 
  # IFACES and IFACE_SRCS with proper values as needed by ADD_PARAVIEW_PLUGIN macro.
  ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
   IFACES IFACE_SRCS 
   CLASS_NAME pqSurfaceLICDisplayPanelDecorator
   PANEL_TYPES pqDisplayProxyEditor 
       # <-- This identifies the panel type(s) to decorate
       # Our decorator will only be instantiated for the panel types indicated here
  )


  ADD_PARAVIEW_PLUGIN( SurfaceLIC "1.0"
    SERVER_MANAGER_XML 
      ${SM_XMLS}
      
    SERVER_MANAGER_SOURCES 
      ${SM_SOURCES}
      
    GUI_INTERFACES 
      ${IFACES} 
    GUI_SOURCES 
      ${MOC_SRCS}
      ${SURFACE_LIC_SOURCES}
      ${UI_SRCS}
      ${IFACE_SRCS}
    )
    
ENDIF (PARAVIEW_BUILD_QT_GUI)
