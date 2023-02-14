# Find PythonQt
#
# Sets PythonQt_FOUND, PythonQt_INCLUDE_DIRS, PythonQt_LIBRARIES and PythonQt_QtAll
# PythonQt::PythonQt_QtAll Target is optional and only provided if the library is found.
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
  if (NOT TARGET PythonQt::PythonQt)
    add_library(PythonQt::PythonQt UNKNOWN IMPORTED)
    set_target_properties(PythonQt::PythonQt
      PROPERTIES
        IMPORTED_LOCATION "${PythonQt_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PythonQt_INCLUDE_DIR}")
  endif ()
  if (PythonQt_QtAll_LIBRARY)
    if (NOT TARGET PythonQt::PythonQt_QtAll)
      add_library(PythonQt::PythonQt_QtAll UNKNOWN IMPORTED)
      set_target_properties(PythonQt::PythonQt_QtAll
        PROPERTIES
          IMPORTED_LOCATION "${PythonQt_QtAll_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${PythonQt_INCLUDE_DIR}")
    endif ()
  endif ()
endif ()
