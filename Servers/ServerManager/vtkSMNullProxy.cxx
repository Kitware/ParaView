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
//-----------------------------------------------------------------------------
vtkSMNullProxy::vtkSMNullProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMNullProxy::~vtkSMNullProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::CreateVTKObjects()
{
  this->SetGlobalID(2); // GlobalId 2 is a reserved one for NULL proxy
  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
