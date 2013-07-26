# Find PythonQt
#
# You can pass PYTHONQT_DIR to set the root directory that
# contains lib/ and include/PythonQt where PythonQt is installed.
#
# Sets PYTHONQT_FOUND, PYTHONQT_INCLUDE_DIRS, PYTHONQT_LIBRARIES
#
# The cache variables are PYTHON_QT_INCLUDE_DIR AND PYTHONQT_LIBRARY.
#

if(PYTHONQT_DIR)
  set(_pythonqt_include_dir "${PYTHONQT_DIR}/include/PythonQt")
  set(_pythonqt_lib_dir "${PYTHONQT_DIR}/lib")
endif()

find_path(PYTHONQT_INCLUDE_DIR PythonQt.h HINTS ${_pythonqt_include_dir} DOC "Path to the PythonQt include directory")
find_library(PYTHONQT_LIBRARY PythonQt HINTS ${_pythonqt_lib_dir} DOC "The PythonQt library")

set(PYTHONQT_INCLUDE_DIRS ${PYTHONQT_INCLUDE_DIR})
set(PYTHONQT_LIBRARIES ${PYTHONQT_LIBRARY})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonQt DEFAULT_MSG PYTHONQT_INCLUDE_DIR PYTHONQT_LIBRARY)

mark_as_advanced(PYTHONQT_INCLUDE_DIR)
mark_as_advanced(PYTHONQT_LIBRARY)
