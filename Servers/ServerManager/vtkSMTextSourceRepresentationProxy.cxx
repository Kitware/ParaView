/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextSourceRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMTextSourceRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::vtkSMTextSourceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::~vtkSMTextSourceRepresentationProxy()
{
}


//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetID() << "SetTextWidgetRepresentation"
    << this->GetSubProxy("TextWidgetRepresentation")->GetID()
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
