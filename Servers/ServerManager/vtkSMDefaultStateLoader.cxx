/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDefaultStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDefaultStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMDefaultStateLoader);
vtkCxxRevisionMacro(vtkSMDefaultStateLoader, "1.1");
//-----------------------------------------------------------------------------
vtkSMDefaultStateLoader::vtkSMDefaultStateLoader()
{
}

//-----------------------------------------------------------------------------
vtkSMDefaultStateLoader::~vtkSMDefaultStateLoader()
{
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDefaultStateLoader::NewProxy(int id)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID csid;
  csid.ID = static_cast<vtkTypeUInt32>(id);
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(pm->GetObjectFromID(csid));
  if (proxy)
    {
    return proxy;
    }
  return this->Superclass::NewProxy(id);
}

//-----------------------------------------------------------------------------
void vtkSMDefaultStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
