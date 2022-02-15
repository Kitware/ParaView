/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNew2DWidgetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNew2DWidgetRepresentationProxy.h"

#include "vtk2DWidgetRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkContextItem.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>
#include <list>

vtkStandardNewMacro(vtkSMNew2DWidgetRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMNew2DWidgetRepresentationProxy::vtkSMNew2DWidgetRepresentationProxy()
  : Superclass()
{
}

//----------------------------------------------------------------------------
vtkSMNew2DWidgetRepresentationProxy::~vtkSMNew2DWidgetRepresentationProxy()
{
  this->ContextItemProxy = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMNew2DWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->ContextItemProxy = this->GetSubProxy("ContextItem");
  if (!this->ContextItemProxy)
  {
    vtkErrorMacro("A representation proxy must be defined as a ContextItem sub-proxy");
    return;
  }
  this->ContextItemProxy->SetLocation(vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT);

  this->Superclass::CreateVTKObjects();

  // Location 0 is for prototype objects !!! No need to send to the server something.
  if (!this->ContextItemProxy || this->Location == 0)
  {
    return;
  }

  // Bind the Widget and the representations on the server side
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetContextItem"
         << VTKOBJECT(this->ContextItemProxy) << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT);

  vtk2DWidgetRepresentation* clientObject =
    vtk2DWidgetRepresentation::SafeDownCast(this->GetClientSideObject());
  this->ContextItem = clientObject->GetContextItem();

  if (this->ContextItem)
  {
    this->ContextItem->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    this->ContextItem->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
    this->ContextItem->AddObserver(vtkCommand::InteractionEvent, this->Observer);
  }

  this->SetupPropertiesLinks();
}

//----------------------------------------------------------------------------
void vtkSMNew2DWidgetRepresentationProxy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
