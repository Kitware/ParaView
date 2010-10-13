/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVLookupTableProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVLookupTableProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMPVLookupTableProxy);
//-----------------------------------------------------------------------------
vtkSMPVLookupTableProxy::vtkSMPVLookupTableProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMPVLookupTableProxy::~vtkSMPVLookupTableProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMPVLookupTableProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPVLookupTableProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  this->Superclass::UpdateVTKObjects(stream);
  this->InvokeCommand("Build");
}

//-----------------------------------------------------------------------------
void vtkSMPVLookupTableProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
