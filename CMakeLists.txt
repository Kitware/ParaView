PROJECT(PARAVIEW) 

FIND_PATH( VTK_BINARY_DIR vtkConfigure.h )

# load in the values from VTK if found
IF ( VTK_BINARY_DIR )
  LOAD_CACHE(${VTK_BINARY_DIR} EXCLUDE
                                BUILD_SHARED_LIBS
                                BUILD_TESTING
                                LIBRARY_OUTPUT_PATH
                                EXECUTABLE_OUTPUT_PATH
                               INCLUDE_INTERNALS
                                VTK_BINARY_PATH
                                VTK_LIBRARY_PATH)
  INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)
  SUBDIRS(Widgets ParaView)
  SUBDIR_DEPENDS(ParaView Widgets)
ENDIF (VTK_BINARY_DIR )

OPTION(BUILD_SHARED_LIBS "Build ParaView with shared libraries." OFF)
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

IF (MPI_LIBRARY)
  LINK_LIBRARIES (${MPI_LIBRARY})
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF (MPI_LIBRARY)

IF (TK_INCLUDE_PATH)
  INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
ENDIF (TK_INCLUDE_PATH)

INCLUDE_DIRECTORIES(
${PARAVIEW_BINARY_DIR} 
${PARAVIEW_SOURCE_DIR}/Widgets
${PARAVIEW_SOURCE_DIR}/ParaView
)

LINK_DIRECTORIES(
${PARAVIEW_BINARY_DIR}/Widgets
${PARAVIEW_BINARY_DIR}/ParaView
)

LINK_DIRECTORIES(
${VTK_LIBRARY_PATH}
)

IF (UNIX)
  LINK_LIBRARIES(${THREAD_LIBS} ${DL_LIBS} -lm)
ENDIF (UNIX)

IF (WIN32)
FIND_PATH( HTML_HELP_PATH htmlhelp.h 
	   "c:/program files/html help workshop"  
	   "c:/program files/html help workshop/include" )

IF (HTML_HELP_PATH)
  INCLUDE_DIRECTORIES(${HTML_HELP_PATH})
ENDIF (HTML_HELP_PATH)


FIND_LIBRARY( HTML_HELP_LIBRARY htmlhelp
	   "c:/program files/html help workshop"  
	   "c:/program files/html help workshop/LIB")

IF (HTML_HELP_LIBRARY)	  
   LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
ENDIF (HTML_HELP_LIBRARY)
ENDIF (WIN32)

SET (PARAVIEW_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/ParaView)

# Include the standard Dart testing module
INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)

# Add the testing directories
IF(BUILD_TESTING)
  IF (DART_ROOT)
    SUBDIRS(ParaView/Testing)
    SUBDIR_DEPENDS(ParaView/Testing ParaView)
  ENDIF (DART_ROOT)
ENDIF(BUILD_TESTING)

SET (LIBRARY_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")
