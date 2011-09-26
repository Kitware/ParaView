
# create a plugin that implements an object panel for a source proxy
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT(PrismClientPlugin)

FIND_PACKAGE(Qt4)
IF(QT4_FOUND)

INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/Plugins/PrismPlugins/Server
)


# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
SET(MOC_INCLUDE_DIRS ${MOC_INCLUDE_DIRS} ${QT_QTDESIGNER_INCLUDE_DIR} ${pqComponents_SOURCE_DIR} )
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")


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

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${RCS_SOURCES}
)
ADD_PARAVIEW_VIEW_MODULE(
                         VIEW_IFACES
                         VIEW_IFACE_SRCS
                         VIEW_TYPE "PrismView"
                         VIEW_NAME "Prism View"
                         VIEW_XML_GROUP views
                         VIEW_XML_NAME PrismView
                         )


ADD_PARAVIEW_ACTION_GROUP(TOOLBARIFACE TOOLBAR_IFACE_SRCS CLASS_NAME PrismToolBarActions
                          GROUP_NAME "ToolBar/Prism")

ADD_PARAVIEW_ACTION_GROUP(MENUBARIFACE MENU_IFACE_SRCS CLASS_NAME PrismMenuActions
                          GROUP_NAME "MenuBar/Prism")

ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR(DISPLAYPANELIFACES DISPLAY_PANEL_IFACE_SRCS CLASS_NAME PrismDisplayPanelDecorator
                          PANEL_TYPES pqDisplayProxyEditor)


SET(SM_PRISM_SRCS
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
ADD_PARAVIEW_PLUGIN(PrismClientPlugin "1.1"
                    SERVER_MANAGER_XML PRISMServerManagerXML.xml
                    GUI_INTERFACES ${VIEW_IFACES} ${TOOLBARIFACE} ${MENUBARIFACE} ${DISPLAYPANELIFACES}  PrismObjectPanels
                    SERVER_MANAGER_SOURCES
                    ${SM_PRISM_SRCS}
                    GUI_SOURCES
                    ${GUI_PRISM_SRCS}
                    ${RCS_SOURCES}
                    ${MOC_SRCS}
                    ${TOOLBAR_IFACE_SRCS}
                    ${MENU_IFACE_SRCS}
                    ${UI_BUILT_SOURCES}
                    ${VIEW_IFACE_SRCS}
                    ${DISPLAY_PANEL_IFACE_SRCS}
                    REQUIRED_ON_CLIENT
)

IF(WIN32)
INSTALL(FILES SESAMEConversions.xml
        DESTINATION ${PV_INSTALL_BIN_DIR}
        COMPONENT Runtime)
ELSE(WIN32)
INSTALL(FILES SESAMEConversions.xml
        DESTINATION ${PV_INSTALL_LIB_DIR}
        COMPONENT Runtime)
ENDIF(WIN32)

IF (BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF (BUILD_TESTING)

ENDIF(QT4_FOUND)
