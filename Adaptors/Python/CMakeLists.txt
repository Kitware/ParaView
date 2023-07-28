# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause

set(python_files
  cp_pythonadaptorscript.py
  pythonadaptordriver.py
)

vtk_module_add_python_package(ParaView::AdaptorsPython
  FILES               ${python_files}
  PACKAGE             "paraview.adaptors"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::AdaptorsPython
  PACKAGES "paraview.adaptors")
