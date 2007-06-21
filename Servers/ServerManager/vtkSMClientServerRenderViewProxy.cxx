/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientServerRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientServerRenderViewProxy.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkSMClientServerRenderViewProxy classes defined.
#include "vtkSMIceTDesktopRenderViewProxy.h"

#define PV_IMPLEMENT_CLIENT_SERVER_WO_ICET
#define vtkSMIceTDesktopRenderViewProxy vtkSMClientServerRenderViewProxy
#include "vtkSMIceTDesktopRenderViewProxy.cxx"
#undef vtkSMIceTDesktopRenderViewProxy

vtkStandardNewMacro(vtkSMClientServerRenderViewProxy);
vtkCxxRevisionMacro(vtkSMClientServerRenderViewProxy, "1.4");

