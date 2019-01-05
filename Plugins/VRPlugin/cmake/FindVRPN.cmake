find_path(VRPN_INCLUDE_DIR
  NAMES vrpn_Tracker.h
  DOC   "VRPN include directory")
mark_as_advanced(VRPN_INCLUDE_DIR)

find_library(VRPN_LIBRARY
  NAME vrpn
  DOC   "VRPN library")
mark_as_advanced(VRPN_LIBRARY)

if (UNIX AND NOT APPLE)
  find_path(VRPN_LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    DOC   "VRPN libusb.h include directory")
  mark_as_advanced(VRPN_LIBUSB_INCLUDE_DIR)
endif ()

# TODO: Version variable? It appears as though it is only in a
# source file in the repository.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VRPN
  REQUIRED_VARS VRPN_INCLUDE_DIR VRPN_LIBUSB_INCLUDE_DIR)

if (VRPN_FOUND)
  set(VRPN_INCLUDE_DIRS "${VRPN_INCLUDE_DIR}" ${VRPN_LIBUSB_INCLUDE_DIR})
  set(VRPN_LIBRARIES "${VRPN_LIBRARY}")
  if (NOT TARGET VRPN::VRPN)
    add_library(VRPN::VRPN UNKNOWN IMPORTED)
    set_target_properties(VRPN::VRPN
      PROPERTIES
        IMPORTED_LOCATION "${VRPN_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${VRPN_INCLUDE_DIRS}")
  endif ()
endif ()
