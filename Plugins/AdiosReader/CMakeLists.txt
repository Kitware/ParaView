#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE})
ENDIF(PARAVIEW_BUILD_QT_GUI)

#------------------------------------------------------------------------------
#          --- We enable Adios only on MPI ParaView build ---
#------------------------------------------------------------------------------
if(PARAVIEW_USE_MPI)
   find_package(MPI)
   #--------------------------------------------------
   # On Windows we don't do anything
   #--------------------------------------------------
   if(WIN32)
      message("The Adios Plugin can not be compiled on Windows.")
   else(WIN32)
      include(FindAdios.cmake)
   endif(WIN32)

   #--------------------------------------------------
   # Real plugin management
   #--------------------------------------------------
   IF(ADIOS_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(
         ${ADIOS_INCLUDE_PATH}
         ${MPI_INCLUDE_PATH})

      ADD_PARAVIEW_PLUGIN( Adios "1.4"
         SERVER_MANAGER_XML Adios.xml
         SERVER_MANAGER_SOURCES vtkAdiosPixieReader.cxx
         GUI_RESOURCE_FILES AdiosGUI.xml)

      TARGET_LINK_LIBRARIES( Adios
         ${ADIOS_READ_LIBRARY} ${MPI_LIBRARIES})

   ENDIF(ADIOS_INCLUDE_PATH)

endif(PARAVIEW_USE_MPI)
