# This file set up Tcl/Tk without using VTK's Tcl/Tk configuration code
# (to avoid setting WRAP_TCL to On).
#
# The following variables need to be defined:
# APPLICATION_BINARY_DIR: application bin dir
#   ex: ${ParaView_BINARY_DIR}
# VTK_TCL_TK_MACROS_MODULE: module where the VTK Tcl/Tk macros are to be found
#   ex: ${ParaView_SOURCE_DIR}/VTK/CMake/vtkTclTkMacros.cmake
# VTK_TCL_TK_INTERNALS_DIR: the path to VTK Tcl/Tk internals directory
#   ex: ${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/internals
# VTK_TCL_TK_WRAPPING_DIR: the path to VTK Tcl/Tk Wrapping directory
#   ex: ${ParaView_SOURCE_DIR}/VTK/Wrapping/Tcl

PROJECT(TclTk)

IF(UNIX)
  SET(TCL_TK_STATIC 1 CACHE INTERNAL "Build with static Tcl/Tk support. TCL_LIBRARY and TK_LIBRARY must point to the corresponding Tcl/Tk static libraries (example, tcl84sx.lib, tk84sx.lib)." FORCE)
  SET(TclTk_BINARY_DIR_TCLBIN "${TclTk_BINARY_DIR}/tcl8.4.5/unix")
  SET(TclTk_BINARY_DIR_TKBIN "${TclTk_BINARY_DIR}/tk8.4.5/unix")

  FOREACH(tcltkdir "${TclTk_BINARY_DIR_TCLBIN}" "${TclTk_BINARY_DIR_TKBIN}")
    IF(EXISTS ${tcltkdir})
    ELSE(EXISTS ${tcltkdir})
      MAKE_DIRECTORY(${tcltkdir})
    ENDIF(EXISTS ${tcltkdir})
  ENDFOREACH(tcltkdir)

  SET(TCLTK_64BIT)
  IF("${CMAKE_C_FLAGS}" MATCHES "-64")
    SET(TCLTK_64BIT 1)
  ENDIF("${CMAKE_C_FLAGS}" MATCHES "-64")

  SET(TCLTK_DEBUG_EXT "")
  IF(CMAKE_BUILD_TYPE MATCHES Debug)
    SET(TCLTK_BUILD_DEBUG 1)
    SET(TCLTK_DEBUG_EXT "g")
  ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

  IF(TCL_TCLSH MATCHES "${TclTk_BINARY_DIR_TCLBIN}")
    SET(TCL_TCLSH "TCL_TCLSH-NOTFOUND" CACHE FILEPATH "Tcl SH" FORCE)
  ENDIF(TCL_TCLSH MATCHES "${TclTk_BINARY_DIR_TCLBIN}")

  SET(TCL_LIBRARY ${TclTk_BINARY_DIR_TCLBIN}/libtcl8.4${TCLTK_DEBUG_EXT}.a 
    CACHE INTERNAL "Path to tcl library" FORCE)
  SET(TCL_STUB_LIBRARY 
    ${TclTk_BINARY_DIR_TCLBIN}/libtclstub8.4${TCLTK_DEBUG_EXT}.a 
    CACHE INTERNAL "Path to tcl stub library" FORCE)
  SET(TCL_INCLUDE_PATH ${TclTk_SOURCE_DIR}/tcl8.4.5/generic 
    CACHE INTERNAL "Path to tcl include" FORCE)
  SET(TK_LIBRARY ${TclTk_BINARY_DIR_TKBIN}/libtk8.4${TCLTK_DEBUG_EXT}.a 
    CACHE INTERNAL "Path to tk library" FORCE)
  SET(TK_STUB_LIBRARY 
    ${TclTk_BINARY_DIR_TKBIN}/libtkstub8.4${TCLTK_DEBUG_EXT}.a 
    CACHE INTERNAL "Path to tk stub library" FORCE)
  SET(TK_INCLUDE_PATH ${TclTk_SOURCE_DIR}/tk8.4.5/generic 
    CACHE INTERNAL "Path to tk include" FORCE)

  CONFIGURE_FILE(${TclTk_SOURCE_DIR}/configure_tcltk.sh.in
    ${TclTk_BINARY_DIR}/configure_tcltk.sh IMMEDIATE @ONLY)
  CONFIGURE_FILE(${TclTk_SOURCE_DIR}/build_tcltk.sh.in
    ${TclTk_BINARY_DIR}/build_tcltk.sh IMMEDIATE @ONLY)

  IF(RUN_CONFIGURE)
    EXEC_PROGRAM( "/bin/sh \"${TclTk_BINARY_DIR}/configure_tcltk.sh\"")
  ENDIF(RUN_CONFIGURE)
  ADD_CUSTOM_TARGET(maketcltk ALL "/bin/sh" "${TclTk_BINARY_DIR}/build_tcltk.sh")

ELSE(UNIX)

  # copy the tcl/tk dll's to Release Debug MinSizeRel and RelWithDebInfo bin 
  # directory so no paths are needed to run this.
  # This includes the Tcl/Tk dll, and eventually some support dll

  IF (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ELSE (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES .)
  ENDIF (CMAKE_CONFIGURATION_TYPES)

  SET(TCL_TK_WIN_DISTRIB_DIR ${TclTk_SOURCE_DIR}/TclTk8.4.5Win)

  IF(BUILD_SHARED_LIBS)
    SET(SUFFIX "")
    FOREACH(config ${CONFIG_TYPES})
      SET(DLLS 
        TclTk8.4.5Win/bin/tcl84.dll 
        TclTk8.4.5Win/bin/tk84.dll
        tkdnd/lib/tkdnd/libtkdnd.dll)
      FOREACH(dll ${DLLS})
        GET_FILENAME_COMPONENT(dllname ${dll} NAME)
        CONFIGURE_FILE(
          ${TclTk_SOURCE_DIR}/${dll}
          ${APPLICATION_BINARY_DIR}/bin/${config}/${dllname}
          COPYONLY)
      ENDFOREACH(dll)
    ENDFOREACH(config)
  ELSE (BUILD_SHARED_LIBS)
    SET(SUFFIX "sx")
  ENDIF (BUILD_SHARED_LIBS)

  SET(TCL_LIBRARY ${TCL_TK_WIN_DISTRIB_DIR}/lib/tcl84${SUFFIX}.lib CACHE 
      INTERNAL "Path to tcl library" FORCE)
  SET(TCL_LIBRARY_DEBUG ${TCL_LIBRARY} CACHE INTERNAL
    "Path to tcl debug library" FORCE)

  SET(TK_LIBRARY ${TCL_TK_WIN_DISTRIB_DIR}/lib/tk84${SUFFIX}.lib CACHE INTERNAL
    "Path to tk library" FORCE)
  SET(TK_LIBRARY_DEBUG ${TK_LIBRARY} CACHE INTERNAL
    "Path to tk debug library" FORCE)

  SET(TCL_INCLUDE_PATH ${TCL_TK_WIN_DISTRIB_DIR}/include CACHE INTERNAL
    "Path to tcl include" FORCE)
  SET(TK_INCLUDE_PATH ${TCL_TK_WIN_DISTRIB_DIR}/include CACHE INTERNAL
    "Path to tk include" FORCE)

ENDIF(UNIX)

# Copy the Tcl/Tk support files
# See vtkTclTkMacros.cmake for full documentation about
# VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR

SET(TCL_TK_COPY_SUPPORT_LIBRARY 1)
SET(TCL_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tcl8.4.5/library)
SET(TK_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tk8.4.5/library)

INCLUDE(${VTK_TCL_TK_MACROS_MODULE})
VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
  "${TCL_SUPPORT_LIBRARY_PATH}"
  "${TK_SUPPORT_LIBRARY_PATH}"
  "${APPLICATION_BINARY_DIR}/lib")

INSTALL_FILES(/lib/tcl8.4 FILES ${TCL_SUPPORT_FILES})
INSTALL_FILES(/lib/tk8.4 FILES ${TK_SUPPORT_FILES})

# More support files

IF (NOT UNIX)
  SET (TKDND_SUPPORT_LIBRARY_PATH "${TclTk_SOURCE_DIR}/tkdnd/lib/tkdnd")
  FOREACH (file "pkgIndex.tcl" "tkDND_Utils.tcl")
    SET (TKDND_SUPPORT_FILES 
         ${TKDND_SUPPORT_FILES} "${TKDND_SUPPORT_LIBRARY_PATH}/${file}")
    CONFIGURE_FILE ("${TKDND_SUPPORT_LIBRARY_PATH}/${file}" 
                    "${APPLICATION_BINARY_DIR}/lib/tkdnd/${file}" COPYONLY)
  ENDFOREACH (file)
  INSTALL_FILES(/lib/tkdnd FILES ${TKDND_SUPPORT_FILES})
ENDIF (NOT UNIX)

# Set TK_INTERNAL_PATH

SET (try_file "")
IF (CYGWIN OR WIN32)
  SET (try_file "tkWinPort.h")
ENDIF (CYGWIN OR WIN32)
IF (APPLE)
  SET (try_file "tkMacOSXPort.h")
ENDIF (APPLE)
IF (try_file)
  VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
  SET (TCL_TK_VERSIOND "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
  FIND_PATH(
    TK_INTERNAL_PATH 
    ${try_file} 
    "${VTK_TCL_TK_INTERNALS_DIR}/tk${TCL_TK_VERSIOND}"
    DOC "The path to the Tk internal headers (${try_file}).")
  MARK_AS_ADVANCED(TK_INTERNAL_PATH)
ENDIF (try_file)

# Take care of the resources

INCLUDE(${VTK_TCL_TK_WRAPPING_DIR}/vtkDetermineTkResources.cmake)
