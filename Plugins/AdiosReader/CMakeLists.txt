include(FindAdios.cmake)

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

INCLUDE(${QT_USE_FILE})


IF( PARAVIEW_USE_MPI )
  find_package(MPI)
ENDIF( PARAVIEW_USE_MPI )

IF(ADIOS_INCLUDE_PATH)

  INCLUDE_DIRECTORIES(
       ${ADIOS_INCLUDE_PATH}
  )
  IF(PARAVIEW_USE_MPI)
      SET(_ADIOS_LIB
           ${ADIOS_READ_LIBRARY}
           ${MPI_LIBRARIES}
      )
  ELSE(PARAVIEW_USE_MPI)
      SET(_ADIOS_LIB ${ADIOS_READ_NO_MPI_LIBRARY} )
  ENDIF(PARAVIEW_USE_MPI)

  ADD_PARAVIEW_PLUGIN(AdiosReaderPlugin "1.0"
    SERVER_MANAGER_XML vtkAdiosReader.xml
    SERVER_MANAGER_SOURCES vtkAdiosReader.cxx
    GUI_RESOURCE_FILES vtkAdiosReaderGUI.xml)

  TARGET_LINK_LIBRARIES(
    AdiosReaderPlugin
    ${_ADIOS_LIB}
    )

ENDIF(ADIOS_INCLUDE_PATH)
