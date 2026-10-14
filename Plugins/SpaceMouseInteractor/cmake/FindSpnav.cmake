#[=======================================================================[.rst:
FindSpnav
---------

Find libspnav, the client library for spacenavd. Set ``SPNAV_ROOT`` to an
installation prefix

Provides the imported target ``Spnav::Spnav``.
#]=======================================================================]

find_path(Spnav_INCLUDE_DIR
  NAMES spnav.h
  HINTS ${SPNAV_ROOT} ENV SPNAV_ROOT
  PATH_SUFFIXES include src)

# A source build has a versioned shared object but no linker symlink until installation.
find_library(Spnav_LIBRARY
 NAMES spnav
 HINTS ${SPNAV_ROOT} ENV SPNAV_ROOT
 PATH_SUFFIXES lib lib64 "")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Spnav
  REQUIRED_VARS Spnav_INCLUDE_DIR Spnav_LIBRARY)

if (Spnav_FOUND AND NOT TARGET Spnav::Spnav)
  add_library(Spnav::Spnav UNKNOWN IMPORTED)
  set_target_properties(Spnav::Spnav PROPERTIES
    IMPORTED_LOCATION "${Spnav_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Spnav_INCLUDE_DIR}")
endif ()

mark_as_advanced(Spnav_INCLUDE_DIR Spnav_LIBRARY)
