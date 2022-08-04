# - Find Gmsh library
# Find the Gmsh includes and library
# This module defines
#  Gmsh_INCLUDE_DIRS: where to find GmshGlobal.h
#  Gmsh_LIBRARIES: the Gmsh library
#  Gmsh_FOUND: if false, do not try to use Gmsh
#  Gmsh_VERSION: The found version of Gmsh

find_path(Gmsh_INCLUDE_DIR
  NAMES
    gmsh.h
  PATHS
    /usr/local/include
    /usr/include)
mark_as_advanced(Gmsh_INCLUDE_DIR)

find_library(Gmsh_LIBRARY
  NAMES
    gmsh libgmsh)
mark_as_advanced(Gmsh_LIBRARY)

if (Gmsh_INCLUDE_DIR)
  if (EXISTS "${Gmsh_INCLUDE_DIR}/gmsh.h")
    file(STRINGS "${Gmsh_INCLUDE_DIR}/gmsh.h" _gmsh_version
      REGEX "GMSH_")
    string(REGEX REPLACE ".*GMSH_API_VERSION_MAJOR *\([0-9]*\).*" "\\1" _gmsh_major "${_gmsh_version}")
    string(REGEX REPLACE ".*GMSH_API_VERSION_MINOR *\([0-9]*\).*" "\\1" _gmsh_minor "${_gmsh_version}")
    string(REGEX REPLACE ".*GMSH_API_VERSION_PATCH *\([0-9]*\).*" "\\1" _gmsh_patch "${_gmsh_version}")
    unset(_gmsh_version)
    if (NOT _gmsh_major STREQUAL "" AND
        NOT _gmsh_minor STREQUAL "" AND
        NOT _gmsh_patch STREQUAL "")
      set(Gmsh_VERSION "${_gmsh_major}.${_gmsh_minor}.${_gmsh_patch}")
    endif ()
    unset(_gmsh_major)
    unset(_gmsh_minor)
    unset(_gmsh_patch)
  else ()
    set(Gmsh_VERSION Gmsh_VERSION-NOTFOUND)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh
  REQUIRED_VARS Gmsh_LIBRARY Gmsh_INCLUDE_DIR
  VERSION_VAR Gmsh_VERSION)

if (Gmsh_FOUND)
  set(Gmsh_LIBRARIES "${Gmsh_LIBRARY}")
  set(Gmsh_INCLUDE_DIRS "${Gmsh_INCLUDE_DIR}")
  if (NOT TARGET Gmsh::Gmsh)
    add_library(Gmsh::Gmsh UNKNOWN IMPORTED)
    set_target_properties(Gmsh::Gmsh
      PROPERTIES
        IMPORTED_LOCATION "${Gmsh_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Gmsh_INCLUDE_DIR}")
  endif ()
endif ()
