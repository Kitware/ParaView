PROJECT(VIEWS) 

SUBDIRS(Widgets ParaView)

FIND_PATH( VTK_BINARY_DIR vtkConfigure.h )
# load in the values from VTK if found
IF ( VTK_BINARY_DIR )
  LOAD_CACHE(${VTK_BINARY_DIR})
  INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)
ENDIF (VTK_BINARY_DIR )

INCLUDE (${CMAKE_ROOT}/Modules/FindMPI.cmake)
IF (MPI_LIBRARY)
  LINK_LIBRARIES (${MPI_LIBRARY})
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF (MPI_LIBRARY)

INCLUDE_DIRECTORIES(
${VIEWS_BINARY_DIR} 
${VIEWS_SOURCE_DIR}/Widgets
${VIEWS_SOURCE_DIR}/ParaView
)

LINK_DIRECTORIES(
${VIEWS_BINARY_DIR}/Widgets
${VIEWS_BINARY_DIR}/ParaView
)

IF (UNIX)
  LINK_LIBRARIES(${THREAD_LIBS} ${DL_LIBS} -lm)
ENDIF (UNIX)

IF (WIN32)
FIND_PATH( HTML_HELP_PATH htmlhelp.h 
	   "c:\program files\html help workshop"  
	   "c:\program files\html help workshop\include" )

IF (HTML_HELP_PATH)
  INCLUDE_DIRECTORIES(${HTML_HELP_PATH})
ENDIF (HTML_HELP_PATH)

FIND_LIBRARY( HTML_HELP_LIBRARY htmlhelp
	   "c:\program files\html help workshop"  
	   "c:\program files\html help workshop\LIB")

IF (HTML_HELP_LIBRARY)	  
   LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
ENDIF (HTML_HELP_LIBRARY)
ENDIF (WIN32)

