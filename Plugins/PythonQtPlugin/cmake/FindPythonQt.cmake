# Find PythonQt
#
# You can pass PYTHONQT_DIR to set the root directory that
# contains lib/ and include/PythonQt where PythonQt is installed.
#
# Sets PYTHONQT_FOUND, PYTHONQT_INCLUDE_DIRS, PYTHONQT_LIBRARIES
#
# The cache variables are PYTHON_QT_INCLUDE_DIR AND PYTHONQT_LIBRARY.
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

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PythonQt
  REQUIRED_VARS PythonQt_INCLUDE_DIR PythonQt_LIBRARY)

if (PythonQt_FOUND)
  set(PythonQt_INCLUDE_DIRS "${PythonQt_INCLUDE_DIR}")
  set(PythonQt_LIBRARIES "${PythonQt_LIBRARY}")
  if (NOT TARGET PythonQt::PythonQt)
    # TODO: Add interface for Python and Qt dependencies.
    add_library(PythonQt::PythonQt UNKNOWN IMPORTED)
    set_target_properties(PythonQt::PythonQt
      PROPERTIES
        IMPORTED_LOCATION "${QtTesting_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PythonQt_INCLUDE_DIR}")
  endif ()
endif ()
