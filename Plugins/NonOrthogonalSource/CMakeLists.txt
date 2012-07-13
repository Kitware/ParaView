#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
ENDIF(PARAVIEW_BUILD_QT_GUI)

ADD_PARAVIEW_PLUGIN(NonOrthogonalSource "1.0"
     SERVER_MANAGER_XML vtkNonOrthogonalSource.xml
     SERVER_MANAGER_SOURCES vtkShearedCubeSource.cxx
)
