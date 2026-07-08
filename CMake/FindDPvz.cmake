find_path(DPvz_INCLUDE_DIR
  NAMES DPvzVtk.h
  PATHS
    "${DPvz_ROOT}/include"
  DOC "DPvz include directory")
mark_as_advanced(DPvz_INCLUDE_DIR)

if (DPVZ_MPI)
  set(_library_name DPvzMpi)
else()
  set(_library_name DPvzSer)
endif()

find_library(DPvz_LIBRARY
  NAMES ${_library_name} lib${_library_name}
  PATHS
    "${DPvz_ROOT}/lib"
  DOC "DPvz library")
mark_as_advanced(DPvz_LIBRARY)
unset(_library_name)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DPvz
  REQUIRED_VARS DPvz_LIBRARY DPvz_INCLUDE_DIR)

if (DPvz_FOUND)
  set(DPvz_INCLUDE_DIRS "${DPvz_INCLUDE_DIR}")
  set(DPvz_LIBRARIES "${DPvz_LIBRARY}")

  if (NOT TARGET DPvz::DPvz)
    add_library(DPvz::DPvz UNKNOWN IMPORTED)
    set_target_properties(DPvz::DPvz PROPERTIES
      IMPORTED_LOCATION "${DPvz_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${DPvz_INCLUDE_DIR}")
  endif ()
endif ()
