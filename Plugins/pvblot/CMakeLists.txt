# This plugin implements scripting that resembles commands from the SEACAS blot
# program.

SET(PYMODULES
  blotish.py
  pvblot.py
  blot_common.py
  number_list_parser.py
  tplot.py
  async_io_helper.py
  timestep_selection.py
  )

SET(MOC_HEADERS
  pqBlotDialog.h
  pqBlotShell.h
  PVBlotPluginActions.h
  )

SET(UI_FILES
  pqBlotDialog.ui
  )

SET(CLIENT_SRCS
  pqBlotDialog.cxx
  pqBlotShell.cxx
  PVBlotPluginActions.cxx
  )

INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

IF (PARAVIEW_BUILD_QT_GUI)

  IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    SET(Qt5_FIND_COMPONENTS Widgets)
    INCLUDE(ParaViewQt5)
  ELSE()
    INCLUDE(${QT_USE_FILE})
  ENDIF()

  INCLUDE_DIRECTORIES(
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${pqCore_SOURCE_DIR}
    ${pqCore_BINARY_DIR}
    ${pqComponents_SOURCE_DIR}
    ${pqComponents_BINARY_DIR}
    ${QtWidgets_SOURCE_DIR}
    ${QtWidgets_BINARY_DIR}
    )

  IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
    QT5_WRAP_UI(UI_SRCS ${UI_FILES})
  ELSE()
    QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
    QT4_WRAP_UI(UI_SRCS ${UI_FILES})
  ENDIF()

  ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS
    CLASS_NAME PVBlotPluginActions
    GROUP_NAME "MenuBar/Tools")

  ADD_PARAVIEW_PLUGIN(pvblot "1.0"
    PYTHON_MODULES ${PYMODULES}
    GUI_INTERFACES ${IFACES}
    GUI_RESOURCES pvblot.qrc
    REQUIRED_ON_CLIENT
    SOURCES ${MOC_SRCS} ${UI_SRCS} ${IFACE_SRCS} ${CLIENT_SRCS}
    )
ELSE ()
  ADD_PARAVIEW_PLUGIN(pvblot "1.0"
    PYTHON_MODULES ${PYMODULES}
    REQUIRED_ON_CLIENT
    )
ENDIF ()

#############################################################################
# Set up a simple shell script to run pvblot from the command line.
GET_TARGET_PROPERTY(PVBLOT_PVPYTHON_EXECUTABLE pvpython LOCATION)
SET(PVBLOT_BLOTISH_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

SET(script_ext)
IF(WIN32 AND NOT CYGWIN)
  SET(script_ext ".bat")
ENDIF()

#############################################################################
# Set up pvblot script for when ParaView is installed.  This includes
# installing the Python source files.
INSTALL(
  FILES ${PYMODULES}
  DESTINATION ${PV_INSTALL_LIB_DIR}
  )

SET(PVBLOT_PVPYTHON_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_BIN_DIR}/pvpython${PV_EXE_SUFFIX}")
SET(PVBLOT_BLOTISH_PATH "${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}")


CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/pvblot${script_ext}.in"
  "${ParaView_BINARY_DIR}/CMake/tmp/pvblot${script_ext}"
  @ONLY
  )


INSTALL(
  FILES "${ParaView_BINARY_DIR}/CMake/tmp/pvblot${script_ext}"
  DESTINATION ${PV_INSTALL_BIN_DIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )
