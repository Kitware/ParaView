# create a plugin that implements an object panel for a source proxy
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
  
# Qt interface
SET( MOC_SRCS )
SET( UI_SRCS )

IF (PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})

  QT4_WRAP_CPP(MOC_SRCS 
    PrismDisplayPanelDecorator.h
    PrismToolBarActions.h
    PrismCore.h
    PrismMenuActions.h
    PrismObjectPanelsImplementation.h
    PrismScaleViewDialog.h
    PrismSurfacePanel.h
    PrismPanel.h
    PrismView.h  
    )
  
  QT4_WRAP_UI(UI_BUILT_SOURCES
      Resources/UI/PrismSurfacePanelWidget.ui
      Resources/UI/PrismPanelWidget.ui
      Resources/UI/PrismViewScalingWidget.ui
      )
  
  SET(UI_RESOURCES
    Resources/Prism.qrc
  )
  
  QT4_ADD_RESOURCES(RCS_SOURCES
    ${UI_RESOURCES}
  )

  SOURCE_GROUP("Resources" FILES ${UI_RESOURCES})
  SOURCE_GROUP("Generated" FILES ${RCS_SOURCES})
  

ADD_PARAVIEW_ACTION_GROUP(
  TOOLBARIFACE TOOLBAR_IFACE_SRCS CLASS_NAME PrismToolBarActions
  GROUP_NAME "ToolBar/Prism")

ADD_PARAVIEW_ACTION_GROUP(
  MENUBARIFACE MENU_IFACE_SRCS CLASS_NAME PrismMenuActions
  GROUP_NAME "MenuBar/Prism")

ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(
  DISPLAYPANELIFACES DISPLAY_PANEL_IFACE_SRCS CLASS_NAME PrismDisplayPanelDecorator
  PANEL_TYPES pqDisplayProxyEditor)

ADD_PARAVIEW_VIEW_OPTIONS(
  VIEWOPTIONS_IFACE VIEWOPTIONS_SRCS
  VIEW_TYPE "PrismView"
  ACTIVE_VIEW_OPTIONS pqActiveRenderViewOptions)
  
ENDIF (PARAVIEW_BUILD_QT_GUI)


set(SERVER_SOURCES
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


ADD_PARAVIEW_VIEW_MODULE(
    VIEW_IFACES
    VIEW_IFACE_SRCS
    VIEW_TYPE "PrismView"
    VIEW_NAME "Prism View"
    VIEW_XML_GROUP views
    VIEW_XML_NAME PrismView
  )

SET(SERVER_MANAGER_SOURCES
    vtkSMPrismDoubleRangeDomain.cxx
    vtkSMPrismViewProxy.cxx
    vtkSMPrismSourceProxy.cxx
  )

SET(GUI_PRISM_SRCS
  PrismDisplayPanelDecorator.cxx
  PrismToolBarActions.cxx
  PrismCore.cxx
  PrismMenuActions.cxx
  PrismObjectPanelsImplementation.cxx
  PrismScaleViewDialog.cxx
  PrismSurfacePanel.cxx
  PrismPanel.cxx
  PrismView.cxx
  )
# create a plugin for this panel
ADD_PARAVIEW_PLUGIN(PrismPlugin "1.1"
  SERVER_MANAGER_XML PRISMServerManagerXML.xml
  GUI_INTERFACES
    ${VIEW_IFACES} ${TOOLBARIFACE} ${MENUBARIFACE} ${DISPLAYPANELIFACES}
    ${VIEWOPTIONS_IFACE}
    PrismObjectPanels
  SERVER_MANAGER_SOURCES
    ${SERVER_MANAGER_SOURCES}
    ${SERVER_SOURCES}
  GUI_SOURCES
   ${GUI_PRISM_SRCS}
   ${RCS_SOURCES}
   ${MOC_SRCS}
   ${TOOLBAR_IFACE_SRCS}
   ${MENU_IFACE_SRCS}
   ${UI_BUILT_SOURCES}
   ${VIEW_IFACE_SRCS}
   ${DISPLAY_PANEL_IFACE_SRCS}
   ${VIEWOPTIONS_SRCS}
  GUI_RESOURCE_FILES
    SESAMEConversions.xml
  REQUIRED_ON_CLIENT REQUIRED_ON_SERVER
)

IF (BUILD_TESTING AND PARAVIEW_BUILD_QT_GUI)
  ADD_SUBDIRECTORY(Testing)
ENDIF ()
