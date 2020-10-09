#[=======================================================================[
FindzSpace
--------

Find zSpace.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``zSpace::zSpace``
if zSpace has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``zSpace_FOUND``
  True if zSpace is found.
``zSpace_INCLUDE_DIRS``
  Include directories for zSpace headers.
``zSpace_LIBRARIES``
  Libraries to link to zSpace.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``zSpace_LIBRARY``
  The libzSpace library file.
``zSpace_INCLUDE_DIR``
  The directory containing ``zSpace.h``.

Hints
^^^^^

Set ``zSpace_DIR`` or ``zSpace_ROOT`` in the environment to specify the
zSpace installation prefix.
example : zSpace_DIR = C:/zSpace/zSpaceSdks/4.0.0.3
#]=======================================================================]
#
# This makes the presumption that you are include zSpace.h like
#
#include "zSpace.h"

find_path(zSpace_INCLUDE_DIR zSpace.h
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  PATH_SUFFIXES
    zSpaceSdks
    include
    Inc
  DOC "Path to the zSpace include directory"
)
mark_as_advanced(zSpace_INCLUDE_DIR)

find_library(zSpace_LIBRARY
  NAMES zSpaceApi64 zSpaceApi
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  PATH_SUFFIXES
    zSpaceSdks
    Lib/x64
    Lib/Win32
  DOC "Path to the zSpace library"
)
mark_as_advanced(zSpace_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zSpace
    REQUIRED_VARS zSpace_LIBRARY zSpace_INCLUDE_DIR)

if (zSpace_FOUND)
    set(zSpace_LIBRARIES ${zSpace_LIBRARY})
    set(zSpace_INCLUDE_DIRS ${zSpace_INCLUDE_DIR})

    if (NOT TARGET zSpace::zSpace)
        add_library(zSpace::zSpace UNKNOWN IMPORTED)
        set_target_properties(zSpace::zSpace PROPERTIES
            IMPORTED_LOCATION "${zSpace_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${zSpace_INCLUDE_DIR}")
    endif ()
endif ()
