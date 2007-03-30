/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNullProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNullProxy.h"

#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkSMNullProxy);
vtkCxxRevisionMacro(vtkSMNullProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMNullProxy::vtkSMNullProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMNullProxy::~vtkSMNullProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::CreateVTKObjects(int vtkNotUsed(numObjects))
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->ObjectsCreated = 1;
  this->GetSelfID(); // this will ensure that the SelfID is assigned properly.

  vtkClientServerID objectId = { 0 };
  this->SetID(0, objectId);
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
