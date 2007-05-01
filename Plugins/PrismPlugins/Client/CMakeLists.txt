
# create a plugin that implements an object panel for a source proxy
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

PROJECT(PrismClientPlugin)


FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
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

QT4_WRAP_CPP(MOC_SRCS PrismToolBarActions.h)

SET(UI_RESOURCES
  Resources/Prism.qrc
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${RCS_SOURCES}
)

# we implement a pqConePanel.h for the ConeSource
ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS CLASS_NAME PrismToolBarActions 
                          GROUP_NAME "ToolBar/PrismActions")

# create a plugin for this panel
ADD_PARAVIEW_PLUGIN(PrismClientPlugin "1.0" 
                    GUI_INTERFACES ${IFACES} 
                    SOURCES PrismToolBarActions.cxx ${RCS_SOURCES} ${MOC_SRCS} ${IFACE_SRCS})
  


