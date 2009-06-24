
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
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/Plugins/PrismPlugins/Server
  ${pqComponents_SOURCE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtChart_SOURCE_DIR}
  ${QtChart_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${QtPython_SOURCE_DIR}
  ${QtPython_BINARY_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
SET(MOC_INCLUDE_DIRS ${MOC_INCLUDE_DIRS} ${QT_QTDESIGNER_INCLUDE_DIR} ${pqComponents_SOURCE_DIR} )
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

SET(KIT Prism)
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator PrismInstantiator_SRCS
  vtkSMPrismCubeAxesRepresentationProxy.cxx
  VTK_EXPORT
  "${CMAKE_CURRENT_BINARY_DIR}" "")




QT4_WRAP_CPP(MOC_SRCS PrismToolBarActions.h PrismCore.h PrismMenuActions.h PrismDisplayProxyEditor.h PrismDisplayPanelsImplementation.h PrismObjectPanelsImplementation.h PrismSurfacePanel.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
    Resources/UI/PrismSurfacePanelWidget.ui)

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
ADD_PARAVIEW_PLUGIN(PrismClientPlugin "1.0" 
                    GUI_INTERFACES ${TOOLBARIFACE} ${MENUBARIFACE} PrismDisplayPanels PrismObjectPanels 
                    SERVER_MANAGER_SOURCES vtkSMPrismCubeAxesRepresentationProxy.cxx
                    GUI_SOURCES 
                    PrismToolBarActions.cxx 
                    PrismCore.cxx 
                    PrismMenuActions.cxx 
                    PrismDisplayProxyEditor.cxx 
                    PrismDisplayPanelsImplementation.cxx 
                    PrismObjectPanelsImplementation.cxx 
                    PrismSurfacePanel.cxx  
                    ${PrismInstantiator_SRCS} 
                    ${RCS_SOURCES} 
                    ${MOC_SRCS} 
                    ${TOOLBAR_IFACE_SRCS} 
                    ${MENU_IFACE_SRCS}
                    ${UI_BUILT_SOURCES}
    
)
  

ENDIF(QT4_FOUND)

