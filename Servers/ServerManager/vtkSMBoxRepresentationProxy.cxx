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

#include "vtkSMSession.h"
#include "vtkSMMessage.h"

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

  // Set the transform
  vtkSMMessage msg;
  msg.set_global_id(this->GlobalID);
  msg.set_location(this->Location);
  VariantList *args = (msg << pvstream::InvokeRequest() << "SetTransform")._arguments;
  Variant* arg = args->add_variant();
  arg->set_type(Variant::PROXY);
  arg->add_proxy_global_id(this->GetSubProxy("Transform")->GetGlobalID());
  this->Session->Invoke(&msg);
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdateVTKObjects()
{
  if (this->InUpdateVTKObjects)
    {
    return;
    }

  int something_changed = this->ArePropertiesModified();

  this->Superclass::UpdateVTKObjects();

  if (something_changed)
    {
    // Set the transform
    vtkSMMessage msg;
    msg.set_global_id(this->GlobalID);
    msg.set_location(this->Location);
    VariantList *args = (msg << pvstream::InvokeRequest() << "SetTransform")._arguments;
    Variant* arg = args->add_variant();
    arg->set_type(Variant::PROXY);
    arg->add_proxy_global_id(this->GetSubProxy("Transform")->GetGlobalID());
    this->Session->Invoke(&msg);
    }
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::UpdatePropertyInformation()
{
  vtkBoxRepresentation* repr = vtkBoxRepresentation::SafeDownCast(
    this->GetClientSideObject());
  vtkTransform* transform = vtkTransform::SafeDownCast(
      this->GetSubProxy("Transform")->GetClientSideObject());
  repr->GetTransform(transform);

  this->Superclass::UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMBoxRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


