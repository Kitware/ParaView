#########################################################################
# Setup Qt
SET(QT_REQUIRED TRUE)
FIND_PACKAGE(Qt4)

IF(QT4_FOUND)
# No warning if Qt not found.  ParaView/Qt does the warning for us

# This ensures that the Qt includes dir etc are set properly.
# We don't need to find the Qt4 package or include the use file
# in any of the SUBDIRS in this directory.
INCLUDE(${QT_USE_FILE})

#########################################################################
# Everything under Application uses VTK/ParaView ServerManager.
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


MACRO(paraview_build_optional_application name comment subdirectory default)
  OPTION(PARAVIEW_BUILD_${name} "Build ${comment}" ${default})
  MARK_AS_ADVANCED(PARAVIEW_BUILD_${name})
  IF(PARAVIEW_BUILD_${name})
    MESSAGE(STATUS "Application: ${comment} enabled")
    ADD_SUBDIRECTORY("${subdirectory}")
  ELSE(PARAVIEW_BUILD_${name})
    MESSAGE(STATUS "Application: ${comment} disabled")
  ENDIF(PARAVIEW_BUILD_${name})
ENDMACRO(paraview_build_optional_application)

paraview_build_optional_application(Client "Standard ParaView Client" Client ON)
paraview_build_optional_application(OverView "OverView Informatics Client" OverView OFF)
paraview_build_optional_application(StreamingParaView "Streaming ParaView Client" StreamingParaView OFF)

ENDIF(QT4_FOUND)
