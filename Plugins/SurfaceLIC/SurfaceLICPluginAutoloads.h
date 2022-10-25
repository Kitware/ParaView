/*=========================================================================

  Program:   ParaView
  Module:    SurfaceLICPluginAutoloads.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This file
 */
#ifndef SurfaceLICPluginAutoloads_h
#define SurfaceLICPluginAutoloads_h

#include "vtkLogger.h"

void SurfaceLICPluginAutoloads()
{
  vtkLog(WARNING, "SurfaceLIC is now built-in ParaView. There is no need to load this plugin.");
}

#endif
