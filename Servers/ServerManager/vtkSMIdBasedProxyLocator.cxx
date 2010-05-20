/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIdBasedProxyLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIdBasedProxyLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkClientServerInterpreter.h"

vtkStandardNewMacro(vtkSMIdBasedProxyLocator);
//----------------------------------------------------------------------------
vtkSMIdBasedProxyLocator::vtkSMIdBasedProxyLocator()
{
}

//----------------------------------------------------------------------------
vtkSMIdBasedProxyLocator::~vtkSMIdBasedProxyLocator()
{
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMIdBasedProxyLocator::NewProxy(int id)
{
  vtkClientServerInterpreter *interp =
    vtkProcessModule::GetProcessModule()->GetInterpreter();

  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(
    interp->GetObjectFromID(vtkClientServerID(id), 1));
  if (proxy)
    {
    proxy->Register(this);
    }
  return (proxy)? proxy : this->Superclass::NewProxy(id);
}

//----------------------------------------------------------------------------
void vtkSMIdBasedProxyLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


