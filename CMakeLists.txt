PROJECT(ParaViewComplete)
SUBDIRS(VTK)
SUBDIR_DEPENDS(ParaView VTK)

SET (LIBRARY_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
      "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
      "Single output directory for building all executables.")

# force these to be ON by default as they are required by ParaView
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes" ON)
OPTION (VTK_USE_PARALLEL "Build the parallel directory classes" ON)
OPTION (VTK_WRAP_TCL "wrap classes into the TCL intepreted language" ON)

OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
IF(NOT BUILD_SHARED_LIBS)
  SET(TCL_TK_STATIC 1)
ENDIF(NOT BUILD_SHARED_LIBS)

OPTION (USE_INCLUDED_TCLTK "Should Tcl/Tk included in ParaViewComplete be used. Turn this off if you want to use your own Tcl/Tk. Note that you need version 8.4.5" ON)

OPTION (PARAVIEW_LINK_XDMF "Build ParaView complete using included Xdmf library" OFF)
IF(PARAVIEW_LINK_XDMF)
  SET(XDMF_BUILD_VTK 1)
  SET(XDMF_WRAP_PYTHON 0)
  SET(XDMF_WRAP_TCL 0)
  SET(VTK_DIR ${ParaViewComplete_BINARY_DIR}/VTK)
  SET(VTK_NO_LIBRARY_DEPENDS 1)
  SUBDIRS(Xdmf)
ENDIF(PARAVIEW_LINK_XDMF)

SUBDIRS(ParaView)

IF(USE_INCLUDED_TCLTK)
  IF(UNIX)
    IF(RUN_CONFIGURE)
      EXEC_PROGRAM(
       "${ParaViewComplete_SOURCE_DIR}/tcl8.4.5/unix/configure --disable-shared"
       ${ParaViewComplete_BINARY_DIR}/tcl8.4.5/unix)
      EXEC_PROGRAM(
       "${ParaViewComplete_SOURCE_DIR}/tk8.4.5/unix/configure --disable-shared"
       ${ParaViewComplete_BINARY_DIR}/tk8.4.5/unix)
    ENDIF(RUN_CONFIGURE)

    SET(TCL_LIBRARY ${ParaViewComplete_BINARY_DIR}/tcl8.4.5/unix/libtcl8.4.a 
       CACHE FILEPATH "Path to tcl library")
    SET(TCL_STUB_LIBRARY 
       ${ParaViewComplete_BINARY_DIR}/tcl8.4.5/unix/libtclstub8.4.a 
       CACHE FILEPATH "Path to tcl stub library")
    SET(TCL_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tcl8.4.5/generic 
        CACHE PATH "Path to tcl include")
    SET(TK_LIBRARY ${ParaViewComplete_BINARY_DIR}/tk8.4.5/unix/libtk8.4.a 
       CACHE FILEPATH "Path to tk library")
    SET(TK_STUB_LIBRARY 
       ${ParaViewComplete_BINARY_DIR}/tk8.4.5/unix/libtkstub8.4.a 
       CACHE FILEPATH "Path to tk stub library")
    SET(TK_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/tk8.4.5/generic 
        CACHE PATH "Path to tk include")
    IF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
      IF (CMAKE_MINOR_VERSION MATCHES [3-9][0-9]*)
        SET(CMAKE_ESCAPES_SC 1)
      ENDIF (CMAKE_MINOR_VERSION MATCHES [3-9][0-9]*)
    ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)

    # On UNIX, Tcl needs the math library.
    IF(UNIX)
      SET(TCL_LIBRARY ${TCL_LIBRARY} -lm)
    ENDIF(UNIX)

    # If using X with Tk, we need to link to X when we link
    # to Tk in order to support static linking.
    IF(CMAKE_HAS_X)
      SET(TK_LIBRARY ${TK_LIBRARY} -lXt ${CMAKE_X_LIBS})
    ENDIF(CMAKE_HAS_X)

    IF (CMAKE_ESCAPES_SC)
      ADD_CUSTOM_TARGET(maketcl ALL 
        cd ${ParaViewComplete_BINARY_DIR}/tcl8.4.5/unix\; "$(MAKE)" )
      ADD_CUSTOM_TARGET(maketk ALL 
        cd ${ParaViewComplete_BINARY_DIR}/tk8.4.5/unix\; "$(MAKE)" )
    ELSE (CMAKE_ESCAPES_SC)
      ADD_CUSTOM_TARGET(maketcl ALL 
        cd ${ParaViewComplete_BINARY_DIR}/tcl8.4.5/unix; "$(MAKE)" )
      ADD_CUSTOM_TARGET(maketk ALL 
        cd ${ParaViewComplete_BINARY_DIR}/tk8.4.5/unix; "$(MAKE)" )
    ENDIF (CMAKE_ESCAPES_SC)
  
  ELSE(UNIX)
    SET(TCL_LIBRARY ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/lib/tcl84.lib
        CACHE FILEPATH "Path to tcl library")
    SET(TCL_STUB_LIBRARY 
       ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/lib/tclstub84.lib
       CACHE FILEPATH "Path to tcl stub library")
    SET(TCL_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/include 
        CACHE PATH "Path to tcl include")
    SET(TK_LIBRARY ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/lib/tk84.lib 
       CACHE FILEPATH "Path to tk library")
    SET(TK_STUB_LIBRARY 
       ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/lib/tkstub84.lib 
       CACHE FILEPATH "Path to tk stub library")
    SET(TK_INCLUDE_PATH ${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/include 
        CACHE PATH "Path to tcl include")
  # copy the tcl/tk dll's to Release Debug MinSizeRel and RelWithDebInfo bin 
  # directory so no paths are needed to run this.
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/tcl84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/Debug/tcl84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/Release/tcl84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/MinSizeRel/tcl84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tcl84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/RelWithDebInfo/tcl84.dll
                    COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/tk84.dll COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/Debug/tk84.dll COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/Release/tk84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/MinSizeRel/tk84.dll 
                   COPYONLY)
    CONFIGURE_FILE(${ParaViewComplete_SOURCE_DIR}/TclTk8.4.5Win/bin/tk84.dll
                   ${ParaViewComplete_BINARY_DIR}/bin/RelWithDebInfo/tk84.dll 
                   COPYONLY)
  ENDIF(UNIX)
ENDIF(USE_INCLUDED_TCLTK)

# Include the standard Dart testing module
INCLUDE (${CMAKE_ROOT}/Modules/FindDart.cmake)

#
# try to find the data files for VTKData
#
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData 
                                       ${VTK_SOURCE_DIR}/../../VTKData 
                                       $ENV{VTK_DATA_ROOT} DOC 
 "If you have VTK data files (used in tests and examples), where is it located?")

MARK_AS_ADVANCED(CLEAR VTK_USE_MPI MPI_EXTRA_LIBRARY MPI_INCLUDE_PATH 
                       MPI_LIBRARY)
MARK_AS_ADVANCED(BUILD_EXAMPLES VTK_WRAP_TCL VTK_WRAP_JAVA VTK_WRAP_PYTHON 
                 VTK_USE_HYBRID VTK_USE_PARALLEL VTK_USE_RENDERING 
                 VTK_USE_VOLUMEPRO VTK_USE_ANSI_STDLIB VTK_USE_X TK_WISH 
                 TCL_TCLSH PARAVIEW_DATA_ROOT TCL_LIBRARY_DEBUG
                 TCL_STUB_LIBRARY_DEBUG TK_LIBRARY_DEBUG TK_STUB_LIBRARY_DEBUG
                 VTK_USE_VIDEO_FOR_WINDOWS VTK_USE_MATROX_IMAGING)
MARK_AS_ADVANCED(USE_INCLUDED_TCLTK TCL_INCLUDE_PATH TCL_LIBRARY 
                 TCL_STUB_LIBRARY TK_INCLUDE_PATH TK_LIBRARY TK_STUB_LIBRARY )

