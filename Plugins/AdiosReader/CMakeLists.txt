#------------------------------------------------------------------------------
# Find and Use ParaView
#------------------------------------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )
ELSE ()
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

#------------------------------------------------------------------------------
# Create client side plugin for any platform
#------------------------------------------------------------------------------
ADD_PARAVIEW_PLUGIN( AdiosClientOnly "1.4"
   SERVER_MANAGER_XML
      Adios.xml
   )

#------------------------------------------------------------------------------
# We enable Adios only on MPI ParaView build
#------------------------------------------------------------------------------
if(PARAVIEW_USE_MPI)

   find_package(MPI)

   #--------------------------------------------------
   # On Windows we don't try to build anything
   #--------------------------------------------------
   if(WIN32)
      message("
        The Adios Plugin can not be compiled on Windows.
        Therefore, only a client side plugin will be created.")
   else()
      include(FindAdios.cmake)
   endif()

   #--------------------------------------------------
   # Real plugin management
   #--------------------------------------------------
   IF(ADIOS_INCLUDE_PATH)
      INCLUDE_DIRECTORIES(SYSTEM
         ${ADIOS_INCLUDE_PATH}
         ${MPI_INCLUDE_PATH})

      ADD_PARAVIEW_PLUGIN( Adios "1.4"
         SERVER_MANAGER_XML
            Adios.xml
         SERVER_MANAGER_SOURCES
            vtkAdiosPixieReader.cxx
         )

      TARGET_LINK_LIBRARIES( Adios
              LINK_PRIVATE ${ADIOS_READ_LIBRARY} ${MPI_LIBRARIES} vtksys)

   ENDIF()

endif()
