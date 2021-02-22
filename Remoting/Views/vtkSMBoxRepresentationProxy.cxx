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
#include "vtkSMSession.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSMBoxRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMBoxRepresentationProxy::vtkSMBoxRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMBoxRepresentationProxy::~vtkSMBoxRepresentationProxy() = default;

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetTransform"
         << VTKOBJECT(this->GetSubProxy("Transform")) << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdateVTKObjects()
{
  if (this->InUpdateVTKObjects)
  {
    return;
  }

  this->Superclass::UpdateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetTransform"
         << VTKOBJECT(this->GetSubProxy("Transform")) << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdatePropertyInformation()
{
  vtkBoxRepresentation* repr = vtkBoxRepresentation::SafeDownCast(this->GetClientSideObject());
  vtkTransform* transform =
    vtkTransform::SafeDownCast(this->GetSubProxy("Transform")->GetClientSideObject());
  repr->GetTransform(transform);

  this->Superclass::UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
