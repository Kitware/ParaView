find_path(VRPN_INCLUDE_DIR
  NAMES vrpn_Tracker.h
  DOC   "VRPN include directory")
mark_as_advanced(VRPN_INCLUDE_DIR)

find_library(VRPN_LIBRARY
  NAMES vrpn
  DOC   "VRPN library")
mark_as_advanced(VRPN_LIBRARY)

set(_vrpn_extra_params)
set(_vrpn_required_vars)

if (VRPN_LIBRARY)
  set(VRPN_INCLUDE_DIRS "${VRPN_INCLUDE_DIR}")
  set(VRPN_LIBRARIES "${VRPN_LIBRARY}")

  vtk_detect_library_type(_vrpn_libtype PATH "${VRPN_LIBRARY}")

  if (NOT _vrpn_libtype STREQUAL "SHARED")
    # In static mode, one should also link with quat library which is
    # built beside vrpn.
    find_library(VRPN_QUAT_LIBRARY
      NAMES quat
      DOC   "VRPN quat library")
    mark_as_advanced(VRPN_QUAT_LIBRARY)
    list(APPEND VRPN_LIBRARIES "${VRPN_QUAT_LIBRARY}")
    list(APPEND _vrpn_required_vars "VRPN_QUAT_LIBRARY")

    if (VRPN_QUAT_LIBRARY)
      if (NOT TARGET VRPN::QUAT)
        add_library(VRPN::QUAT UNKNOWN IMPORTED)
        set_target_properties(VRPN::QUAT
          PROPERTIES
            IMPORTED_LOCATION "${VRPN_QUAT_LIBRARY}")
        set(_vrpn_extra_params INTERFACE_LINK_LIBRARIES "VRPN::QUAT")
      endif ()
    endif ()
  endif ()

  if (NOT TARGET VRPN::VRPN)
    add_library(VRPN::VRPN UNKNOWN IMPORTED)
    set_target_properties(VRPN::VRPN
      PROPERTIES
        IMPORTED_LOCATION "${VRPN_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${VRPN_INCLUDE_DIR}"
        ${_vrpn_extra_params})
  endif ()

  unset(_vrpn_libtype)
  unset(_vrpn_extra_params)
endif ()

# TODO: Version variable? It appears as though it is only in a
# source file in the repository.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VRPN
  REQUIRED_VARS VRPN_INCLUDE_DIR VRPN_LIBRARY ${_vrpn_required_vars})

unset(_vrpn_required_vars)
