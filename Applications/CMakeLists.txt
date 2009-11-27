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
# OverView is disabled until we upgrade it to ParaView Branding
#paraview_build_optional_application(OverView "OverView Informatics Client" OverView OFF)

IF(BUILD_SHARED_LIBS)
  paraview_build_optional_application(StreamingParaView "Streaming ParaView Application" StreamingParaView OFF)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_SHARED_LIBS)
  paraview_build_optional_application(AdaptiveParaView "Adaptive ParaView Application" AdaptiveParaView OFF)
ENDIF(BUILD_SHARED_LIBS)

ENDIF(QT4_FOUND)
