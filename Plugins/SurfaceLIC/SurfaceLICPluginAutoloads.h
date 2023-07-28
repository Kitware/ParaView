// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This file
 */
#ifndef SurfaceLICPluginAutoloads_h
#define SurfaceLICPluginAutoloads_h

#include "vtkLogger.h"

void SurfaceLICPluginAutoloads()
{
  // PARAVIEW_DEPRECATED_IN_5_11_0
  vtkLog(WARNING, "SurfaceLIC is now built-in ParaView. There is no need to load this plugin.");
}

#endif
