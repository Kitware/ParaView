PROJECT(PARAVIEW) 
INCLUDE_REGULAR_EXPRESSION("^(vtk).*$")

SET (LIBRARY_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

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
    FIND_FILE (VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
    MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  ELSE(USE_VTK_FILE)
    SET (PARAVIEW_CAN_BUILD 0)
    IF (CMAKE_MAJOR_VERSION MATCHES "^1$")
      IF (CMAKE_MINOR_VERSION MATCHES "^0$")
        MESSAGE("Warning. VTK might be found on your system as different flavours: installed VTK or built VTK. Please make sure that the VTK_INSTALL_PATH or VTK_BINARY_PATH setting reflects which VTK you are planning to use, then set ONE of the USE_INSTALLED_VTK or USE_BUILT_VTK setting to ON.")
      ENDIF (CMAKE_MINOR_VERSION MATCHES "^0$")
    ENDIF (CMAKE_MAJOR_VERSION MATCHES "^1$")
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

 SET (KWWIDGETS_SOURCE_DIR ${PARAVIEW_SOURCE_DIR}/Widgets)
 SET (KWWIDGETS_BINARY_DIR ${PARAVIEW_BINARY_DIR}/Widgets)
 SUBDIRS(KWCommon Widgets Utilities ParaView Demos)

 # Provide option for building packages.
 OPTION(PARAVIEW_BUILD_PACKAGES
        "Build provided ParaView packages. (EXPERIMENTAL)" OFF)
 MARK_AS_ADVANCED(PARAVIEW_BUILD_PACKAGES)
 IF(PARAVIEW_BUILD_PACKAGES)
  SUBDIRS(Packages)
 ENDIF(PARAVIEW_BUILD_PACKAGES)

 SUBDIR_DEPENDS(ParaView Widgets)
 SUBDIRS(Config)
 IF (BUILD_DOCUMENTATION)
  SUBDIRS(Documentation)
 ENDIF (BUILD_DOCUMENTATION)

 OPTION(BUILD_SHARED_LIBS "Build ParaView with shared libraries." OFF)
 OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
 OPTION(PV_USE_SGI_PIPES "Use SGI rendering pipes." OFF)
 MARK_AS_ADVANCED(PV_USE_SGI_PIPES)

 IF (PV_USE_SGI_PIPES)
  ADD_DEFINITIONS(-DPV_USE_SGI_PIPES)
 ENDIF (PV_USE_SGI_PIPES)

 IF (VTK_USE_MPI)
  IF (MPI_LIBRARY)
    LINK_LIBRARIES (${MPI_LIBRARY})
  ENDIF (MPI_LIBRARY)
  IF (MPI_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_INCLUDE_PATH)
  IF (MPI_EXTRA_LIBRARY)
    LINK_LIBRARIES (${MPI_EXTRA_LIBRARY})
  ENDIF (MPI_EXTRA_LIBRARY)
 ENDIF (VTK_USE_MPI)

 IF (TK_INCLUDE_PATH)
   INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
 ENDIF (TK_INCLUDE_PATH)

 INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR} 
                     ${PARAVIEW_SOURCE_DIR}/KWCommon
                     ${PARAVIEW_SOURCE_DIR}/Widgets
                     ${PARAVIEW_SOURCE_DIR}/ParaView)

 LINK_DIRECTORIES(${PARAVIEW_BINARY_DIR}/KWCommon
                  ${PARAVIEW_BINARY_DIR}/Widgets
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
   ENABLE_TESTING()
   SUBDIRS(ParaView/Testing)
   SUBDIR_DEPENDS(ParaView/Testing ParaView)
 ENDIF(BUILD_TESTING)

 INCLUDE (${PARAVIEW_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
 INCLUDE (${PARAVIEW_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

 INSTALL_FILES(/share/ParaView/Data .bin Data/Data/combq Data/Data/combxyz)
 INSTALL_FILES(/share/ParaView/Data .vtk Data/Data/blow)

ENDIF (PARAVIEW_CAN_BUILD)
