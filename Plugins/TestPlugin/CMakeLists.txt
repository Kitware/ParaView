#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
  )
ELSE ()
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

ADD_PARAVIEW_PLUGIN(PacMan "1.0"
  SERVER_MANAGER_XML PacMan.xml)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
