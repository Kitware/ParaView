PROJECT(PARAVIEW) 
INCLUDE_REGULAR_EXPRESSION("^(vtk).*$")

SET (LIBRARY_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

SET (PARAVIEW_DATA_ROOT ${PARAVIEW_SOURCE_DIR}/Data CACHE PATH "Where the Data, Baseline and Utility directories are located.")

SET (PARAVIEW_CAN_BUILD 1)

IF (WIN32)

 FIND_PATH( HTML_HELP_PATH htmlhelp.h 
 	   "c:/program files/html help workshop"  
	   "c:/program files/html help workshop/include" 
           "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/include" )

 IF (HTML_HELP_PATH)
   INCLUDE_DIRECTORIES(${HTML_HELP_PATH})
 ELSE (HTML_HELP_PATH)
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please install html help." 
           "You can find html help on the msdn web site here:\n"
           "http://msdn.microsoft.com/\n"
"http://msdn.microsoft.com/library/default.asp?url=/library/en-us/htmlhelp/html/vsconwhtshw.asp")
 ENDIF (HTML_HELP_PATH)


 FIND_LIBRARY( HTML_HELP_LIBRARY htmlhelp
              "c:/program files/html help workshop"  
	      "c:/program files/html help workshop/LIB"
              "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/lib")

 IF (HTML_HELP_LIBRARY)	  
   LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
 ELSE (HTML_HELP_LIBRARY)	  
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please set HTML_HELP_PATH")
 ENDIF (HTML_HELP_LIBRARY)

ENDIF (WIN32)

IF(ParaViewComplete_BINARY_DIR)  
  INCLUDE (${VTK_SOURCE_DIR}/vtkCMakeOptions.cmake)
  LINK_DIRECTORIES(${VTK_LIBRARY_PATH})
  INCLUDE(${ParaViewComplete_BINARY_DIR}/VTK/UseVTK.cmake)
ELSE(ParaViewComplete_BINARY_DIR)
  # include module to find vtk
  INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
  # if vtk was found include the use vtk file
  IF (USE_VTK_FILE)
    INCLUDE(${USE_VTK_FILE})
  ELSE(USE_VTK_FILE)
    SET (PARAVIEW_CAN_BUILD 0)
    MESSAGE("ParaView requires VTK to build. Please set VTK_BINARY_PATH to directory where vtkConfigure.h and UseVTK.cmake are found.")
  ENDIF (USE_VTK_FILE)
ENDIF(ParaViewComplete_BINARY_DIR)

IF (PARAVIEW_CAN_BUILD)
 IF ( NOT VTK_WRAP_TCL )
   MESSAGE("ParaView requires VTK to be built with Tcl bindings. Please rebuild VTK with Tcl bindings and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF ( NOT VTK_WRAP_TCL )

 IF (NOT VTK_USE_HYBRID)
   MESSAGE("ParaView requires VTK to be built with hybrid support. Please rebuild VTK with hybrid and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_HYBRID)

 IF (NOT VTK_USE_RENDERING)
   MESSAGE("ParaView requires VTK to be built with rendering support. Please rebuild VTK with rendering and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_RENDERING)

 IF (NOT VTK_USE_PARALLEL)
   MESSAGE("ParaView requires VTK to be built with parallel support. Please rebuild VTK with parallel and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_PARALLEL)
ENDIF (PARAVIEW_CAN_BUILD)

IF (PARAVIEW_CAN_BUILD)

 SUBDIRS(Widgets Utilities/Expat ParaView Demos)
 SUBDIR_DEPENDS(ParaView Widgets Utilities/Expat)
 SUBDIRS(Config Documentation)

 OPTION(BUILD_SHARED_LIBS "Build ParaView with shared libraries." OFF)
 OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

 IF (VTK_USE_MPI)
  IF (MPI_LIBRARY)
    LINK_LIBRARIES (${MPI_LIBRARY})
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_LIBRARY)
  IF (MPI_EXTRA_LIBRARY)
    LINK_LIBRARIES (${MPI_EXTRA_LIBRARY})
  ENDIF (MPI_EXTRA_LIBRARY)
 ENDIF (VTK_USE_MPI)

 IF (TK_INCLUDE_PATH)
   INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
 ENDIF (TK_INCLUDE_PATH)

 INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR} 
                     ${PARAVIEW_SOURCE_DIR}/Widgets
                     ${PARAVIEW_SOURCE_DIR}/ParaView)

 LINK_DIRECTORIES(${PARAVIEW_BINARY_DIR}/Widgets
                  ${PARAVIEW_BINARY_DIR}/ParaView)

 LINK_DIRECTORIES(${VTK_LIBRARY_PATH})

 IF (UNIX)
   LINK_LIBRARIES(${THREAD_LIBS} ${DL_LIBS} -lm)
 ENDIF (UNIX)


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

 INCLUDE (${PARAVIEW_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
 INCLUDE (${PARAVIEW_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

 INSTALL_FILES(/share/ParaView/Data .bin Data/Data/combq Data/Data/combxyz)
 INSTALL_FILES(/share/ParaView/Data .vtk Data/Data/blow)

ENDIF (PARAVIEW_CAN_BUILD)
