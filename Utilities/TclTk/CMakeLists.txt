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

IF(NOT TclTk_INSTALL_BIN_DIR)
  SET(TclTk_INSTALL_BIN_DIR /bin)
ENDIF(NOT TclTk_INSTALL_BIN_DIR)
IF(NOT TclTk_INSTALL_LIB_DIR)
  SET(TclTk_INSTALL_LIB_DIR /lib/TclTk)
ENDIF(NOT TclTk_INSTALL_LIB_DIR)
IF(APPLICATION_BINARY_DIR)
  IF(NOT DEFINED TclTk_BIN_DIR)
    SET(TclTk_BIN_DIR "${APPLICATION_BINARY_DIR}/bin")
  ENDIF(NOT DEFINED TclTk_BIN_DIR)
  IF(NOT DEFINED TclTk_LIB_DIR)
    SET(TclTk_LIB_DIR "${APPLICATION_BINARY_DIR}/lib")
  ENDIF(NOT DEFINED TclTk_LIB_DIR)
ENDIF(APPLICATION_BINARY_DIR)

IF(UNIX)
  SET(VTK_TCL_TK_STATIC 1 CACHE INTERNAL "Build with static Tcl/Tk support. TCL_LIBRARY and TK_LIBRARY must point to the corresponding Tcl/Tk static libraries (example, tcl84sx.lib, tk84sx.lib)." FORCE)
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

  STRING(COMPARE EQUAL "${TCL_TCLSH}" "${TclTk_BINARY_DIR_TCLBIN}" SAME_TCL_DIR)
  IF(SAME_TCL_DIR)
    SET(TCL_TCLSH "TCL_TCLSH-NOTFOUND" CACHE FILEPATH "Tcl SH" FORCE)
  ENDIF(SAME_TCL_DIR)

  SET(TCL_LIBRARY "${TclTk_BINARY_DIR_TCLBIN}/libtcl8.4${TCLTK_DEBUG_EXT}.a"
    CACHE INTERNAL "Path to tcl library" FORCE)
  SET(TCL_STUB_LIBRARY 
    "${TclTk_BINARY_DIR_TCLBIN}/libtclstub8.4${TCLTK_DEBUG_EXT}.a"
    CACHE INTERNAL "Path to tcl stub library" FORCE)
  SET(TCL_INCLUDE_PATH "${TclTk_SOURCE_DIR}/tcl8.4.5/generic"
    CACHE INTERNAL "Path to tcl include" FORCE)
  SET(TK_LIBRARY "${TclTk_BINARY_DIR_TKBIN}/libtk8.4${TCLTK_DEBUG_EXT}.a"
    CACHE INTERNAL "Path to tk library" FORCE)
  SET(TK_STUB_LIBRARY
    "${TclTk_BINARY_DIR_TKBIN}/libtkstub8.4${TCLTK_DEBUG_EXT}.a"
    CACHE INTERNAL "Path to tk stub library" FORCE)
  SET(TK_INCLUDE_PATH "${TclTk_SOURCE_DIR}/tk8.4.5/generic"
    CACHE INTERNAL "Path to tk include" FORCE)
  FIND_PACKAGE(X11)
  GET_FILENAME_COMPONENT(TCL_X11_LIBRARY_PATH "${X11_X11_LIB}" PATH)

  CONFIGURE_FILE("${TclTk_SOURCE_DIR}/configure_tcltk.sh.in"
    "${TclTk_BINARY_DIR}/configure_tcltk.sh" IMMEDIATE @ONLY)
  CONFIGURE_FILE("${TclTk_SOURCE_DIR}/build_tcltk.sh.in"
    "${TclTk_BINARY_DIR}/build_tcltk.sh" IMMEDIATE @ONLY)

  IF(RUN_CONFIGURE)
    EXEC_PROGRAM( "/bin/sh \"${TclTk_BINARY_DIR}/configure_tcltk.sh\"")
  ENDIF(RUN_CONFIGURE)
  ADD_CUSTOM_TARGET(maketcltk ALL "/bin/sh" "${TclTk_BINARY_DIR}/build_tcltk.sh")

  SET(VTK_TCL_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tcl8.4.5/library)
  SET(VTK_TK_SUPPORT_LIBRARY_PATH ${TclTk_SOURCE_DIR}/tk8.4.5/library)

ELSE(UNIX)

  # copy the tcl/tk dll's to Release Debug MinSizeRel and RelWithDebInfo bin 
  # directory so no paths are needed to run this.
  # This includes the Tcl/Tk dll, and eventually some support dll

  IF (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ELSE (CMAKE_CONFIGURATION_TYPES)
    SET (CONFIG_TYPES .)
  ENDIF (CMAKE_CONFIGURATION_TYPES)

  SET(TCL_TK_WIN_VERSION "8.4.13")
  SET(TCL_TK_WIN_DISTRIB_DIR 
    "${TclTk_SOURCE_DIR}/TclTk${TCL_TK_WIN_VERSION}Win")

  IF(BUILD_SHARED_LIBS)
    SET(SUFFIX "")
    SET(SUFFIX_DEBUG "g")
  ELSE (BUILD_SHARED_LIBS)
    SET(SUFFIX "sx")
    SET(SUFFIX_DEBUG "sgx")
    IF("${CMAKE_CXX_FLAGS_RELEASE}" MATCHES "/MT")
      SET(SUFFIX "s")
    ENDIF("${CMAKE_CXX_FLAGS_RELEASE}" MATCHES "/MT")
    IF("${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "/MT")
      SET(SUFFIX_DEBUG "sg")
    ENDIF("${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "/MT")
  ENDIF (BUILD_SHARED_LIBS)

  IF(NOT DEFINED TCL_LIBRARY)
    SET(TCL_LIBRARY ${TCL_TK_WIN_DISTRIB_DIR}/lib/tcl84${SUFFIX}.lib
      CACHE INTERNAL "Path to tcl library" FORCE)
  ENDIF(NOT DEFINED TCL_LIBRARY)
  IF(NOT DEFINED TCL_LIBRARY_DEBUG)
    SET(TCL_LIBRARY_DEBUG ${TCL_TK_WIN_DISTRIB_DIR}/lib/tcl84${SUFFIX_DEBUG}.lib
      CACHE INTERNAL "Path to tcl debug library" FORCE)
  ENDIF(NOT DEFINED TCL_LIBRARY_DEBUG)

  IF(NOT DEFINED TK_LIBRARY)
    SET(TK_LIBRARY ${TCL_TK_WIN_DISTRIB_DIR}/lib/tk84${SUFFIX}.lib
      CACHE INTERNAL "Path to tk library" FORCE)
  ENDIF(NOT DEFINED TK_LIBRARY)
  IF(NOT DEFINED TK_LIBRARY_DEBUG)
    SET(TK_LIBRARY_DEBUG ${TCL_TK_WIN_DISTRIB_DIR}/lib/tk84${SUFFIX_DEBUG}.lib
      CACHE INTERNAL "Path to tk debug library" FORCE)
  ENDIF(NOT DEFINED TK_LIBRARY_DEBUG)

  IF(NOT DEFINED TCL_INCLUDE_PATH)
    SET(TCL_INCLUDE_PATH ${TCL_TK_WIN_DISTRIB_DIR}/include
      CACHE INTERNAL "Path to tcl include" FORCE)
  ENDIF(NOT DEFINED TCL_INCLUDE_PATH)
  IF(NOT DEFINED TK_INCLUDE_PATH)
    SET(TK_INCLUDE_PATH ${TCL_TK_WIN_DISTRIB_DIR}/include
      CACHE INTERNAL "Path to tk include" FORCE)
  ENDIF(NOT DEFINED TK_INCLUDE_PATH)

  IF(NOT DEFINED TCL_TCLSH)
    SET(TCL_TCLSH ${TCL_TK_WIN_DISTRIB_DIR}/bin/tclsh84.exe
      CACHE INTERNAL "Path to Tcl shell" FORCE)
  ENDIF(NOT DEFINED TCL_TCLSH)

  IF(NOT DEFINED TK_WISH)
    SET(TK_WISH ${TCL_TK_WIN_DISTRIB_DIR}/bin/wish84.exe
      CACHE INTERNAL "Path to Tcl shell" FORCE)
  ENDIF(NOT DEFINED TK_WISH)

  SET(VTK_TCL_SUPPORT_LIBRARY_PATH 
    "${TclTk_SOURCE_DIR}/tcl${TCL_TK_WIN_VERSION}/library")
  SET(VTK_TK_SUPPORT_LIBRARY_PATH 
    "${TclTk_SOURCE_DIR}/tk${TCL_TK_WIN_VERSION}/library")

  IF(BUILD_SHARED_LIBS)
    SET(DLLS
      "TclTk${TCL_TK_WIN_VERSION}Win/bin/tcl84.dll"
      "TclTk${TCL_TK_WIN_VERSION}Win/bin/tk84.dll"
      "TclTk${TCL_TK_WIN_VERSION}Win/bin/msvcrt.dll"
      tkdnd/lib/tkdnd/libtkdnd.dll)
    # If VTK uses Tcl, there is *no* choice but installing the debug libs
    # as well. VTK makes its choice regarding what to link against at
    # link time, picking from either TCL_LIBRARY or (TCL_LIBRARY_DEBUG for
    # example.
    IF(VTK_WRAP_TCL)
      SET(DLLS ${DLLS}
        "TclTk${TCL_TK_WIN_VERSION}Win/bin/tcl84g.dll"
        "TclTk${TCL_TK_WIN_VERSION}Win/bin/tk84g.dll"
        "TclTk${TCL_TK_WIN_VERSION}Win/bin/msvcrtd.dll")
    ENDIF(VTK_WRAP_TCL)
    FOREACH(dll ${DLLS})
      GET_FILENAME_COMPONENT(dllname ${dll} NAME)
      IF(TclTk_BIN_DIR)
        FOREACH(config ${CONFIG_TYPES})
          CONFIGURE_FILE(
            ${TclTk_SOURCE_DIR}/${dll}
            ${TclTk_BIN_DIR}/${config}/${dllname}
            COPYONLY)
        ENDFOREACH(config)
      ENDIF(TclTk_BIN_DIR)
      IF(NOT TclTk_INSTALL_NO_RUNTIME)
        INSTALL_FILES(${TclTk_INSTALL_BIN_DIR} 
          FILES ${TclTk_SOURCE_DIR}/${dll})
      ENDIF(NOT TclTk_INSTALL_NO_RUNTIME)
    ENDFOREACH(dll)
  ENDIF (BUILD_SHARED_LIBS)

ENDIF(UNIX)

# Copy the Tcl/Tk support files
# See vtkTclTkMacros.cmake for full documentation about
# VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR
# VTK_TCL_SUPPORT_LIBRARY_PATH and VTK_TK_SUPPORT_LIBRARY_PATH are set above

SET(VTK_TCL_TK_COPY_SUPPORT_LIBRARY 1 CACHE INTERNAL "ParaView links against its own version of Tcl/TK" FORCE)

INCLUDE(${VTK_TCL_TK_MACROS_MODULE})
IF(TclTk_LIB_DIR)
  VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
    "${VTK_TCL_SUPPORT_LIBRARY_PATH}"
    "${VTK_TK_SUPPORT_LIBRARY_PATH}"
    "${TclTk_LIB_DIR}")
ENDIF(TclTk_LIB_DIR)

IF(NOT TclTk_INSTALL_NO_RUNTIME)
  VTK_COPY_TCL_TK_SUPPORT_FILES(
    "${VTK_TCL_SUPPORT_LIBRARY_PATH}"
    "${TclTk_INSTALL_LIB_DIR}/tcl8.4"
    "${VTK_TK_SUPPORT_LIBRARY_PATH}"
    "${TclTk_INSTALL_LIB_DIR}/tk8.4"
    INSTALL)
ENDIF(NOT TclTk_INSTALL_NO_RUNTIME)

# More support files

IF (NOT UNIX)
  SET (TKDND_SUPPORT_LIBRARY_PATH "${TclTk_SOURCE_DIR}/tkdnd/lib/tkdnd")
  FOREACH (file "pkgIndex.tcl" "tkDND_Utils.tcl")
    SET (TKDND_SUPPORT_FILES 
      ${TKDND_SUPPORT_FILES} "${TKDND_SUPPORT_LIBRARY_PATH}/${file}")
       IF(TclTk_LIB_DIR)
         CONFIGURE_FILE ("${TKDND_SUPPORT_LIBRARY_PATH}/${file}" 
           "${TclTk_LIB_DIR}/tkdnd/${file}" COPYONLY)
       ENDIF(TclTk_LIB_DIR)
     ENDFOREACH (file)
  IF(NOT TclTk_INSTALL_NO_RUNTIME)
    INSTALL_FILES(${TclTk_INSTALL_LIB_DIR}/tkdnd FILES ${TKDND_SUPPORT_FILES})
  ENDIF(NOT TclTk_INSTALL_NO_RUNTIME)
ENDIF (NOT UNIX)

# Set TK_INTERNAL_PATH

IF(VTK_TCL_TK_INTERNALS_DIR)
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
ENDIF(VTK_TCL_TK_INTERNALS_DIR)

# Take care of the resources
IF(VTK_TCL_TK_WRAPPING_DIR)
  INCLUDE(${VTK_TCL_TK_WRAPPING_DIR}/vtkDetermineTkResources.cmake)
ENDIF(VTK_TCL_TK_WRAPPING_DIR)
