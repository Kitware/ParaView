# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(python_files
  fv_coprocess.py
  se_coprocess.py
)

vtk_module_add_python_package(ParaView::AdaptorsCamPython
  FILES               ${python_files}
  PACKAGE             "paraview.adaptors.cam"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::AdaptorsCamPython
  PACKAGES "paraview.adaptors.cam")
