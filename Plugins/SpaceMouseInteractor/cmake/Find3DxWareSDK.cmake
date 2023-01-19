#[=======================================================================[
Find3DxWareSDK
--------

Find 3DxWareSDK.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``3Dconnexion::3DxWareSDK``
if 3DxWareSDK has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``3DxWareSDK_FOUND``
  True if 3DxWareSDK is found.
``3DxWareSDK_INCLUDE_DIRS``
  Include directories for 3DxWareSDK headers.
``3DxWareSDK_LIBRARIES``
  Libraries to link to 3DxWareSDK.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``3DxWareSDK_LIBRARY``
  The lib3DxWareSDK library file.
``3DxWareSDK_INCLUDE_DIR``
  The directory containing ``3DxWareSDK.h``.

Hints
^^^^^

Set ``3DxWareSDK_DIR`` or ``3DxWareSDK_ROOT`` in the environment to specify the
3DxWareSDK installation prefix.
example : 3DxWareSDK_DIR = C:/projects/3DxWare_SDK_v4-0-2_r17624
#]=======================================================================]
#
# This makes the presumption that you include navlib.h like
#
#include "navlib/navlib.h"

find_path(3DxWareSDK_INCLUDE_DIR navlib/navlib.h
  HINTS
    ${3DxWareSDK_ROOT}
    ENV 3DxWareSDK_DIR
    ENV 3DxWareSDK_ROOT
  PATH_SUFFIXES
    inc
    include
    Headers
  DOC "Path to the 3DxWareSDK include directory"
)
mark_as_advanced(3DxWareSDK_INCLUDE_DIR)

# windows and apple names.
find_library(3DxWareSDK_LIBRARY
  NAMES TDxNavLib 3DconnexionNavlib
  HINTS
    ${3DxWareSDK_ROOT}
    ENV 3DxWareSDK_DIR
    ENV 3DxWareSDK_ROOT
  PATH_SUFFIXES
    lib/x64
    lib/x86
    lib
    ""
  DOC "Path to the 3DxWareSDK library"
)
mark_as_advanced(3DxWareSDK_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(3DxWareSDK
    REQUIRED_VARS 3DxWareSDK_LIBRARY 3DxWareSDK_INCLUDE_DIR)

if (3DxWareSDK_FOUND)
    set(3DxWareSDK_LIBRARIES ${3DxWareSDK_LIBRARY})
    if (APPLE)
      # CMake should be treating this as a -framework, but we need to
      # list the lib explicitly
      set(3DxWareSDK_LIBRARIES ${3DxWareSDK_LIBRARY}/3DconnexionNavlib)
    endif()
    set(3DxWareSDK_INCLUDE_DIRS ${3DxWareSDK_INCLUDE_DIR})

    if (NOT TARGET 3Dconnexion::3DxWareSDK)
        add_library(3Dconnexion::3DxWareSDK UNKNOWN IMPORTED)
        set_target_properties(3Dconnexion::3DxWareSDK PROPERTIES
            IMPORTED_LOCATION "${3DxWareSDK_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${3DxWareSDK_INCLUDE_DIR}")
    endif ()
endif ()
