# Find PythonQt
#
# Sets PythonQt_FOUND, PythonQt_INCLUDE_DIRS, PythonQt_LIBRARIES
#
# The cache variables are PythonQt_INCLUDE_DIR AND PythonQt_LIBRARY.
#

find_path(PythonQt_INCLUDE_DIR
  NAMES PythonQt.h
  PATH_SUFFIXES PythonQt
  DOC   "Path to the PythonQt include directory")
mark_as_advanced(PythonQt_INCLUDE_DIR)

find_library(PythonQt_LIBRARY
  NAMES PythonQt
  DOC   "The PythonQt library")
mark_as_advanced(PythonQt_LIBRARY)

find_library(PythonQt_QtAll_LIBRARY
  NAMES PythonQt_QtAll
  DOC   "The PythonQt_QtAll library")
mark_as_advanced(PythonQt_QtAll_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PythonQt
  REQUIRED_VARS PythonQt_INCLUDE_DIR PythonQt_LIBRARY)

if (PythonQt_FOUND)
  set(PythonQt_INCLUDE_DIRS "${PythonQt_INCLUDE_DIR}")
  set(PythonQt_LIBRARIES "${PythonQt_LIBRARY};${PythonQt_QtAll_LIBRARY}")
  # TODO: Add interface for Python and Qt dependencies.
  if (NOT TARGET PythonQt::PythonQt)
    add_library(PythonQt::PythonQt UNKNOWN IMPORTED)
    set_target_properties(PythonQt::PythonQt
      PROPERTIES
        IMPORTED_LOCATION "${PythonQt_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PythonQt_INCLUDE_DIR}")
  endif ()
  if (NOT TARGET PythonQt::PythonQt_QtAll)
    add_library(PythonQt::PythonQt_QtAll UNKNOWN IMPORTED)
    set_target_properties(PythonQt::PythonQt_QtAll
      PROPERTIES
        IMPORTED_LOCATION "${PythonQt_QtAll_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PythonQt_INCLUDE_DIR}")
  endif ()
endif ()
