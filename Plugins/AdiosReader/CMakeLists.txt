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

IF(ADIOS_INCLUDE_PATH)

  INCLUDE_DIRECTORIES(
       ${ADIOS_INCLUDE_PATH}
  )

  IF(PARAVIEW_USE_MPI)
     find_package(MPI)

     INCLUDE_DIRECTORIES(
         ${MPI_INCLUDE_PATH}
     )

     ADD_PARAVIEW_PLUGIN(AdiosReaderPluginMPI "1.0"
        SERVER_MANAGER_XML vtkAdiosReader.xml
        SERVER_MANAGER_SOURCES vtkAdiosReader.cxx
        GUI_RESOURCE_FILES vtkAdiosReaderGUI.xml)

     TARGET_LINK_LIBRARIES(
        AdiosReaderPluginMPI
        ${ADIOS_READ_LIBRARY} ${MPI_LIBRARIES})

  ELSE(PARAVIEW_USE_MPI) # ----------- NO MPI ---------------

     ADD_PARAVIEW_PLUGIN(AdiosReaderPlugin "1.0"
        SERVER_MANAGER_XML vtkAdiosReader.xml
        SERVER_MANAGER_SOURCES vtkAdiosReader.cxx
        GUI_RESOURCE_FILES vtkAdiosReaderGUI.xml)

     SET_PROPERTY(
        TARGET AdiosReaderPlugin
        APPEND PROPERTY COMPILE_DEFINITIONS "ADIOS_NO_MPI")

     TARGET_LINK_LIBRARIES(
        AdiosReaderPlugin
        ${ADIOS_READ_NO_MPI_LIBRARY})

  ENDIF(PARAVIEW_USE_MPI)

  ADD_EXECUTABLE(ShowMetaData ShowMetaData.cxx)
  TARGET_LINK_LIBRARIES(ShowMetaData ${ADIOS_READ_NO_MPI_LIBRARY})
  SET_PROPERTY(
        TARGET ShowMetaData
        APPEND PROPERTY COMPILE_DEFINITIONS "ADIOS_NO_MPI")

  ADD_EXECUTABLE(TestRealData TestRealData.cxx)
  TARGET_LINK_LIBRARIES(TestRealData ${ADIOS_READ_NO_MPI_LIBRARY} vtkCommon vtkGraphics)
  SET_PROPERTY(
        TARGET TestRealData
        APPEND PROPERTY COMPILE_DEFINITIONS "ADIOS_NO_MPI")

  ADD_EXECUTABLE(TestRealDataAdiosLib TestRealDataAdiosLib.cxx)
  TARGET_LINK_LIBRARIES(TestRealDataAdiosLib ${ADIOS_READ_NO_MPI_LIBRARY} vtkCommon )
  SET_PROPERTY(
        TARGET TestRealDataAdiosLib
        APPEND PROPERTY COMPILE_DEFINITIONS "ADIOS_NO_MPI")

ENDIF(ADIOS_INCLUDE_PATH)
