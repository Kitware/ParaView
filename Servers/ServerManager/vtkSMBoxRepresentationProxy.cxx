/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoxRepresentationProxy.h"

#include "vtkBoxRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSMBoxRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMBoxRepresentationProxy::vtkSMBoxRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBoxRepresentationProxy::~vtkSMBoxRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetTransform"
          << this->GetSubProxy("Transform")->GetID()
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->GetConnectionID(),
    this->GetServers(), 
    stream);
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  if (this->InUpdateVTKObjects)
    {
    return;
    }

  int something_changed = this->ArePropertiesModified();

  this->Superclass::UpdateVTKObjects(stream);

  if (something_changed)
    {
    stream  << vtkClientServerStream::Invoke
            << this->GetID()
            << "SetTransform"
            << this->GetSubProxy("Transform")->GetID()
            << vtkClientServerStream::End;
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdatePropertyInformation()
{
  vtkBoxRepresentation* repr = vtkBoxRepresentation::SafeDownCast(
    this->GetClientSideObject());
  vtkTransform* transform = vtkTransform::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(
      this->GetSubProxy("Transform")->GetID()));
  repr->GetTransform(transform);

  this->Superclass::UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


