# - Find Gettext run-time library and tools.
# This module finds the GNU gettext run-time library (LGPL), include paths and 
# associated tools (GPL). This code sets the following variables:
#  GETTEXT_INCLUDE_DIR         = path(s) to gettext's include files
#  GETTEXT_LIBRARIES           = the libraries to link against to use gettext
#  GETTEXT_INTL_LIBRARY        = path to gettext's intl library
#  GETTEXT_RUNTIME_FOUND       = true if runtime libs were found (intl)
#  GETTEXT_INFO_MSG            = information string about gettext
#  GETTEXT_XGETTEXT_EXECUTABLE = xgettext tool
#  GETTEXT_MSGINIT_EXECUTABLE  = msginit tool
#  GETTEXT_MSGMERGE_EXECUTABLE = msgmerge tool
#  GETTEXT_MSGCAT_EXECUTABLE   = msgcat tool
#  GETTEXT_MSGCONV_EXECUTABLE  = msgconv tool
#  GETTEXT_MSGFMT_EXECUTABLE   = msgfmt tool
#  GETTEXT_TOOLS_FOUND         = true if all the tools were found
#  GETTEXT_FOUND               = true if both runtime and tools were found
# As a convenience, the following variables can be set before including
# this module to make its life easier:
#  GETTEXT_SEARCH_PATH         = list of path to search gettext components for
# --------------------------------------------------------------------------
# As a convenience, try to find everything as soon as we set any one of
# the cache variables.

MACRO(GETTEXT_FIND_POTENTIAL_DIRS)

  SET(potential_bin_dirs)
  SET(potential_lib_dirs)
  SET(potential_include_dirs)
  FOREACH(filepath 
      "${GETTEXT_INTL_LIBRARY}"
      "${GETTEXT_XGETTEXT_EXECUTABLE}"
      "${GETTEXT_MSGINIT_EXECUTABLE}"
      "${GETTEXT_MSGMERGE_EXECUTABLE}"
      "${GETTEXT_MSGCAT_EXECUTABLE}"
      "${GETTEXT_MSGCONV_EXECUTABLE}"
      "${GETTEXT_MSGFMT_EXECUTABLE}"
      )
    GET_FILENAME_COMPONENT(path "${filepath}" PATH)
    SET(potential_bin_dirs ${potential_bin_dirs} "${path}/../bin")
    SET(potential_lib_dirs ${potential_lib_dirs} "${path}/../lib")
    SET(potential_include_dirs ${potential_include_dirs} "${path}/../include")
  ENDFOREACH(filepath)

  FOREACH(path 
      "${GETTEXT_INCLUDE_DIR}"
      "${GETTEXT_SEARCH_PATH}"
      )
    SET(potential_bin_dirs ${potential_bin_dirs} "${path}/../bin")
    SET(potential_lib_dirs ${potential_lib_dirs} "${path}/../lib")
    SET(potential_include_dirs ${potential_include_dirs} "${path}/../include")
  ENDFOREACH(path)

ENDMACRO(GETTEXT_FIND_POTENTIAL_DIRS)

# --------------------------------------------------------------------------
# Find the runtime lib

MACRO(GETTEXT_FIND_RUNTIME_LIBRARY)

  SET(GETTEXT_RUNTIME_FOUND 1)

  # The gettext intl include dir (libintl.h)
  
  FIND_PATH(GETTEXT_INCLUDE_DIR 
    libintl.h 
    ${potential_include_dirs}
    DOC "Path to gettext include directory (where libintl.h can be found)")
  MARK_AS_ADVANCED(GETTEXT_INCLUDE_DIR)
  IF(NOT GETTEXT_INCLUDE_DIR)
    SET(GETTEXT_RUNTIME_FOUND 0)
  ENDIF(NOT GETTEXT_INCLUDE_DIR)

  SET(GETTEXT_LIBRARIES)

  # The gettext intl library
  # Some Unix system (like Linux) have gettext right into libc

  IF(WIN32)
    SET(HAVE_GETTEXT 0)
  ELSE(WIN32)
    INCLUDE(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(gettext HAVE_GETTEXT)
  ENDIF(WIN32)
  IF(NOT HAVE_GETTEXT)
    FIND_LIBRARY(GETTEXT_INTL_LIBRARY 
      NAMES intl 
      PATHS ${potential_lib_dirs}
      DOC "Path to gettext intl library")
    MARK_AS_ADVANCED(GETTEXT_INTL_LIBRARY)
    IF(NOT GETTEXT_INTL_LIBRARY)
      SET(GETTEXT_RUNTIME_FOUND 0)
    ELSE(NOT GETTEXT_INTL_LIBRARY)
      SET(GETTEXT_LIBRARIES ${GETTEXT_LIBRARIES} ${GETTEXT_INTL_LIBRARY})
    ENDIF(NOT GETTEXT_INTL_LIBRARY)
  ENDIF(NOT HAVE_GETTEXT)

  # The gettext asprintf library
  # Actually not useful as it does not seem to exist on Unix

#   IF(WIN32)
#     FIND_LIBRARY(GETTEXT_ASPRINTF_LIBRARY 
#       NAMES asprintf 
#       PATHS ${potential_lib_dirs}
#       DOC "Gettext asprintf library")
#     MARK_AS_ADVANCED(GETTEXT_ASPRINTF_LIBRARY)
#     IF(NOT GETTEXT_ASPRINTF_LIBRARY)
#       SET(GETTEXT_RUNTIME_FOUND 0)
#     ELSE(NOT GETTEXT_ASPRINTF_LIBRARY)
#       SET(GETTEXT_LIBRARIES ${GETTEXT_LIBRARIES} ${GETTEXT_ASPRINTF_LIBRARY})
#     ENDIF(NOT GETTEXT_ASPRINTF_LIBRARY)
#   ENDIF(WIN32)

ENDMACRO(GETTEXT_FIND_RUNTIME_LIBRARY)

# --------------------------------------------------------------------------
# Find the tools

MACRO(GETTEXT_FIND_TOOLS)
  SET(GETTEXT_TOOLS_FOUND 1)
  FOREACH(tool
      xgettext
      msginit
      msgmerge
      msgcat
      msgconv
      msgfmt
      )
    STRING(TOUPPER ${tool} tool_upper)
    FIND_PROGRAM(GETTEXT_${tool_upper}_EXECUTABLE
      NAMES ${tool} 
      PATHS ${potential_bin_dirs}
      DOC "Path to gettext ${tool} tool")
    MARK_AS_ADVANCED(GETTEXT_${tool_upper}_EXECUTABLE)
    IF(NOT GETTEXT_${tool_upper}_EXECUTABLE)
      SET(GETTEXT_TOOLS_FOUND 0)
    ENDIF(NOT GETTEXT_${tool_upper}_EXECUTABLE)
  ENDFOREACH(tool)
ENDMACRO(GETTEXT_FIND_TOOLS)
  
# --------------------------------------------------------------------------
# Some convenient info about gettext, where to get it, etc.

SET(GETTEXT_INFO_MSG "More information about gettext can be found at http://directory.fsf.org/gettext.html.")
IF(WIN32)
  SET(GETTEXT_INFO_MSG "${GETTEXT_INFO_MSG} Windows users can download gettext-runtime-0.13.1.bin.woe32.zip (LGPL), gettext-tools-0.13.1.bin.woe32.zip (GPL) as well as libiconv-1.9.1.bin.woe32.zip (LGPL) from any GNU mirror (say, http://mirrors.kernel.org/gnu/gettext/ and http://mirrors.kernel.org/gnu/libiconv/), unpack the archives in the same directory, then set GETTEXT_INTL_LIBRARY to 'lib/intl.lib' in and GETTEXT_INCLUDE_DIR to 'include' in that directory.")
ENDIF(WIN32)

# --------------------------------------------------------------------------
# Found ?

GETTEXT_FIND_POTENTIAL_DIRS()
GETTEXT_FIND_RUNTIME_LIBRARY()
GETTEXT_FIND_TOOLS()

# Try again with new potential dirs now that we may have found the runtime
# or the tools

GETTEXT_FIND_POTENTIAL_DIRS()
IF(NOT GETTEXT_RUNTIME_FOUND)
  GETTEXT_FIND_RUNTIME_LIBRARY()
ENDIF(NOT GETTEXT_RUNTIME_FOUND)
IF(NOT GETTEXT_TOOLS_FOUND)
  GETTEXT_FIND_TOOLS()
ENDIF(NOT GETTEXT_TOOLS_FOUND)

IF(GETTEXT_RUNTIME_FOUND AND GETTEXT_TOOLS_FOUND)
  SET(GETTEXT_FOUND 1)
ELSE(GETTEXT_RUNTIME_FOUND AND GETTEXT_TOOLS_FOUND)
  SET(GETTEXT_FOUND 0)
ENDIF(GETTEXT_RUNTIME_FOUND AND GETTEXT_TOOLS_FOUND)

IF(NOT GETTEXT_FOUND AND NOT Gettext_FIND_QUIETLY AND Gettext_FIND_REQUIRED)
  MESSAGE(FATAL_ERROR "Could not find gettext runtime library and tools for internationalization purposes.\n\n${GETTEXT_INFO_MSG}")
ENDIF(NOT GETTEXT_FOUND AND NOT Gettext_FIND_QUIETLY AND Gettext_FIND_REQUIRED)
