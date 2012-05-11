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

if(${CMAKE_VERSION} VERSION_LESS 2.8.7 AND NOT WIN32)
  include(Qt4Macros-CMake2.8.7)
endif()

#########################################################################
# Everything under Application uses VTK/ParaView ServerManager.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
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

paraview_build_optional_application(ParaView "Standard ParaView Client" ParaView ON)

ENDIF(QT4_FOUND)
