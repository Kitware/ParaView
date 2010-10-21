
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


QT4_WRAP_CPP(MOC_SRCS PrismToolBarActions.h PrismCore.h PrismMenuActions.h PrismDisplayProxyEditor.h PrismDisplayPanelsImplementation.h PrismObjectPanelsImplementation.h PrismSurfacePanel.h PrismPanel.h PrismCubeAxesEditorDialog.h )

QT4_WRAP_UI(UI_BUILT_SOURCES
    Resources/UI/PrismSurfacePanelWidget.ui
    Resources/UI/PrismDisplayProxyEditor.ui
    Resources/UI/PrismCubeAxesEditorDialog.ui
    Resources/UI/PrismPanelWidget.ui)

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

ADD_PARAVIEW_ACTION_GROUP(TOOLBARIFACE TOOLBAR_IFACE_SRCS CLASS_NAME PrismToolBarActions 
                          GROUP_NAME "ToolBar/Prism")

ADD_PARAVIEW_ACTION_GROUP(MENUBARIFACE MENU_IFACE_SRCS CLASS_NAME PrismMenuActions 
                          GROUP_NAME "MenuBar/Prism")
# create a plugin for this panel
ADD_PARAVIEW_PLUGIN(PrismClientPlugin "1.1" 
                    SERVER_MANAGER_XML PRISMServerManagerXML.xml
                    GUI_INTERFACES ${TOOLBARIFACE} ${MENUBARIFACE} PrismDisplayPanels PrismObjectPanels
                    SERVER_MANAGER_SOURCES vtkSMPrismCubeAxesRepresentationProxy.cxx vtkSMPrismDoubleRangeDomain.cxx
                    GUI_SOURCES
                    PrismToolBarActions.cxx
                    PrismCore.cxx
                    PrismMenuActions.cxx
                    PrismDisplayProxyEditor.cxx
                    PrismCubeAxesEditorDialog.cxx
                    PrismDisplayPanelsImplementation.cxx
                    PrismObjectPanelsImplementation.cxx
                    PrismSurfacePanel.cxx
                    PrismPanel.cxx
                    ${RCS_SOURCES}
                    ${MOC_SRCS}
                    ${TOOLBAR_IFACE_SRCS}
                    ${MENU_IFACE_SRCS}
                    ${UI_BUILT_SOURCES}
                    REQUIRED_ON_CLIENT
)

IF (BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF (BUILD_TESTING)

ENDIF(QT4_FOUND)
