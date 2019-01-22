find_path(Vapor_INCLUDE_DIR
  NAMES vaporinternal/common.h
  DOC   "NCAR Vapor include directory")
mark_as_advanced(Vapor_INCLUDE_DIR)

find_library(Vapor_VDF_LIBRARY
  NAMES vdf
  DOC   "NCAR Vapor Data Format library")
mark_as_advanced(Vapor_VDF_LIBRARY)

set(_Vapor_vdf_library_dir)
if (Vapor_VDF_LIBRARY)
  get_filename_component(_Vapor_vdf_library_dir "${Vapor_VDF_LIBRARY}" DIRECTORY)
endif ()

find_library(Vapor_COMMON_LIBRARY
  NAMES common
  HINTS ${_Vapor_vdf_library_dir}
  DOC   "NCAR Vapor common library")
mark_as_advanced(Vapor_COMMON_LIBRARY)
unset(_Vapor_vdf_library_dir)

# TODO: Version variable?

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vapor
  REQUIRED_VARS Vapor_INCLUDE_DIR Vapor_VDF_LIBRARY Vapor_COMMON_LIBRARY)

if (Vapor_FOUND)
  # There might be netcdf and expat include directories required here as well.
  set(Vapor_INCLUDE_DIRS "${Vapor_INCLUDE_DIR}")
  set(Vapor_LIBRARIES "${Vapor_VDF_LIBRARY};${Vapor_COMMON_LIBRARY}")

  if (NOT TARGET Vapor::common)
    add_library(Vapor::common UNKNOWN IMPORTED)
    set_target_properties(Vapor::common
      PROPERTIES
        IMPORTED_LOCATION "${Vapor_COMMON_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vapor_INCLUDE_DIR}")
  endif ()

  if (NOT TARGET Vapor::vdf)
    add_library(Vapor::vdf UNKNOWN IMPORTED)
    set_target_properties(Vapor::vdf
      PROPERTIES
        IMPORTED_LOCATION "${Vapor_VDF_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "Vapor::common")
  endif ()
endif ()
