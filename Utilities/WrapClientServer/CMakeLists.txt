# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
if (NOT TARGET ParaView::WrapClientServer)
  vtk_module_add_executable(ParaView::WrapClientServer
    DEVELOPMENT
    vtkWrapClientServer.c)
endif ()
