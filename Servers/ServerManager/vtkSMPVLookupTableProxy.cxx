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
  this->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);
}

//-----------------------------------------------------------------------------
vtkSMPVLookupTableProxy::~vtkSMPVLookupTableProxy()
{
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
