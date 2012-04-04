/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiServerSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiServerSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVMultiServerDataSource.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"

#include <assert.h>


//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMultiServerSourceProxy);
//---------------------------------------------------------------------------
vtkSMMultiServerSourceProxy::vtkSMMultiServerSourceProxy()
{
}

//---------------------------------------------------------------------------
vtkSMMultiServerSourceProxy::~vtkSMMultiServerSourceProxy()
{
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  // Notify the VTK object as well
  vtkPVMultiServerDataSource* clientObj =
      vtkPVMultiServerDataSource::SafeDownCast(this->GetClientSideObject());
  clientObj->Modified();

  // Propagate the dirty flag as regular proxy
  this->Superclass::MarkDirty(modifiedProxy);
}
//---------------------------------------------------------------------------
void vtkSMMultiServerSourceProxy::SetExternalProxy(vtkSMSourceProxy* proxyFromAnotherServer, int port)
{
  vtkPVMultiServerDataSource* clientObj =
      vtkPVMultiServerDataSource::SafeDownCast(this->GetClientSideObject());

  clientObj->SetExternalProxy(proxyFromAnotherServer, port);

  // Create dependency
  proxyFromAnotherServer->AddConsumer(this->GetProperty("DependencyLink"), this);

  // Mark dirty
  this->MarkDirty(proxyFromAnotherServer);
}
