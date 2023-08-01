vtk_module_third_party(
  INTERNAL
    LICENSE_FILES 
      "vtknvpipe/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2017, NVIDIA CORPORATION"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/nvpipe.git@for/paraview-master-20180312"
    VERSION       
      "master"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE nvpipe
    TARGETS nvpipe
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_nvpipe.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_nvpipe.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_nvpipe.h")
