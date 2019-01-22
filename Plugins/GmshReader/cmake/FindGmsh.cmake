# - Find Gmsh library
# Find the Gmsh includes and library
# This module defines
#  Gmsh_INCLUDE_DIRS: where to find GmshGlobal.h
#  Gmsh_LIBRARIES: the Gmsh library
#  Gmsh_FOUND: if false, do not try to use Gmsh
#  Gmsh_VERSION: The found version of Gmsh

find_path(Gmsh_INCLUDE_DIR
  NAMES
    GmshGlobal.h
  PATHS
    /usr/local/include
    /usr/include
  PATH_SUFFIXES
    gmsh)
mark_as_advanced(Gmsh_INCLUDE_DIR)

find_library(Gmsh_LIBRARY
  NAMES
    gmsh libgmsh
  PATH_SUFFIXES
    gmsh)
mark_as_advanced(Gmsh_LIBRARY)

if (Gmsh_INCLUDE_DIR)
  if (EXISTS "${Gmsh_INCLUDE_DIR}/GmshVersion.h")
    file(READ "${Gmsh_INCLUDE_DIR}/GmshVersion.h" _gmsh_version
      REGEX "GMSH_VERSION")
    string(REGEX REPLACE "#define GMSH_VERSION *\"([0-9,.]*).*\"" Gmsh_VERSION "${_gmsh_version}")
    unset(_gmsh_version)
  else ()
    set(Gmsh_VERSION Gmsh_VERSION-NOTFOUND)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh
  REQUIRED_VARS Gmsh_LIBRARY Gmsh_INCLUDE_DIR
  VERSION_VAR Gmsh_VERSION)

if (Gmsg_FOUND)
  set(Gmsh_LIBRARIES "${Gmsh_LIBRARY}")
  set(Gmsh_INCLUDE_DIRS "${Gmsh_INCLUDE_DIR}")
  if (NOT TARGET Gmsh::Gmsh)
    add_library(Gmsh::Gmsh UNKNOWN IMPORTED)
    set_target_properties(TARGET Gmsh::Gmsh
      PROPERTIES
        IMPORTED_LOCATION "${Gmsh_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Gmsh_INCLUDE_DIR}")
  endif ()
endif ()
