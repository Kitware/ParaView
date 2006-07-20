/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMViewModuleProxy);
vtkCxxRevisionMacro(vtkSMViewModuleProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMViewModuleProxy::vtkSMViewModuleProxy()
{
  this->SetDisplayXMLName("GenericViewDisplay");
}

//-----------------------------------------------------------------------------
vtkSMViewModuleProxy::~vtkSMViewModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMViewModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

