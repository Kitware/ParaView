#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
   FIND_PACKAGE(Qt4)
   IF(QT4_FOUND)
      INCLUDE(${QT_USE_FILE})
   ENDIF(QT4_FOUND)
ENDIF(PARAVIEW_BUILD_QT_GUI)

SET(Plugin_SRCS
  vtkForceTime.cxx
)

ADD_PARAVIEW_PLUGIN( ForceTime "1.0"
  SERVER_MANAGER_SOURCES ${Plugin_SRCS}
  SERVER_MANAGER_XML ForceTimeServer.xml
  GUI_RESOURCE_FILES ForceTimeClient.xml
)

TARGET_LINK_LIBRARIES(ForceTime vtkFiltering)
