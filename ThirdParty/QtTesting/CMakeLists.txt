# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

# ParaView can no longer support QT_TESTING_WITH_PYTHON due to Matplotlib
# initializing Python.
set(QT_TESTING_WITH_PYTHON FALSE)

set(BUILD_EXAMPLES OFF)
set(QtTesting_QT_VERSION "${PARAVIEW_QT_MAJOR_VERSION}")
set(QtTesting_INSTALL_BIN_DIR "${_vtk_build_RUNTIME_DESTINATION}")
set(QtTesting_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkqttesting")
set(QtTesting_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
set(QtTesting_INSTALL_CMAKE_DIR "${_vtk_build_CMAKE_DESTINATION}")
set(QtTesting_INSTALL_NO_DEVELOPMENT TRUE)
set(QT_TESTING_EVENT_PLAYBACK_DELAY 100)
set(QtTesting_ENABLE_EXAMPLES OFF)
set(QtTesting_ENABLE_TESTING OFF)
set(QT_TESTING_INSTALL_EXPORT_NAME "ParaView")
set(QTTESTING_BUILD_AS_VTK_MODULE ON)
set(BUILD_TESTING OFF)

vtk_module_third_party_internal(
  LICENSE_FILES 
    "vtkqttesting/Copyright.txt"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2005-2022 Kitware, inc."
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/third-party/paraview/qttesting@dfd965fa829f25707ab7e14b9d4a18238ddc4bdc"
  VERSION       
    "master")
