# - Find Gmsh library
# Find the Gmsh includes and library
# This module defines
#  Gmsh_INCLUDE_DIR: where to find GmshGlobal.h
#  Gmsh_LIBRARY: the Gmsh library
#  Gmsh_FOUND: if false, do not try to use Gmsh
#  Gmsh_VERSION: The found version of Gmsh

find_path(Gmsh_INCLUDE_DIR NAMES GmshGlobal.h
  PATHS
  /usr/include/
  /usr/local/include/
  PATH_SUFFIXES
  gmsh
)

find_library(Gmsh_LIBRARY NAMES gmsh libgmsh
  PATHS
  /usr/lib
  /usr/local/lib
  /usr/lib64
  /usr/local/lib64
  PATH_SUFFIXES
  gmsh
)

if(Gmsh_INCLUDE_DIR)
  set(Gmsh_INCLUDE_DIRS ${Gmsh_INCLUDE_DIR})
  set(VersionFile GmshVersion.h)
  if(EXISTS ${Gmsh_INCLUDE_DIR}/${VersionFile})
    file(READ ${Gmsh_INCLUDE_DIR}/GmshVersion.h GMSH_VERSION_FILE)
    string(REGEX MATCH "\#define GMSH_VERSION *\"([0-9,.]*)\"" GMSH_VERSION_STRING ${GMSH_VERSION_FILE})
    set(Gmsh_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "Gmsh Version")
  else()
    message(SEND_ERROR "Could not find " ${VersionFile} " in " ${Gmsh_INCLUDE_DIR}
                       ". Check path or reconfigure Gmsh with -DENABLE_INTERNAL_DEVELOPER_API=ON")
  endif()
  if(Gmsh_LIBRARY)
    set(Gmsh_LIBRARIES ${Gmsh_LIBRARY})
  endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set Gmsh_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmsh
  FOUND_VAR Gmsh_FOUND
  REQUIRED_VARS Gmsh_LIBRARY Gmsh_INCLUDE_DIR
  VERSION_VAR Gmsh_VERSION)

mark_as_advanced(
  Gmsh_INCLUDE_DIR
  Gmsh_LIBRARY
)
