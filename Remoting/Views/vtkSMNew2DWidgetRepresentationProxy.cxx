// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMNew2DWidgetRepresentationProxy.h"

#include "vtk2DWidgetRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkContextItem.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMContextItemWidgetProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>
#include <list>

vtkStandardNewMacro(vtkSMNew2DWidgetRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMNew2DWidgetRepresentationProxy::vtkSMNew2DWidgetRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMNew2DWidgetRepresentationProxy::~vtkSMNew2DWidgetRepresentationProxy() = default;

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

  // Location NONE is for prototype objects ! No need to send to the server something.
  if (!this->ContextItemProxy || this->Location == vtkPVSession::NONE)
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

//-----------------------------------------------------------------------------
void vtkSMNew2DWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  this->InvokeEvent(event);

  vtkSMContextItemWidgetProxy* widgetProxy =
    vtkSMContextItemWidgetProxy::SafeDownCast(this->ContextItemProxy);

  if (event == vtkCommand::StartInteractionEvent)
  {
    this->ContextItemProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (widgetProxy)
    {
      widgetProxy->OnStartInteraction();
    }
  }
  else if (event == vtkCommand::InteractionEvent)
  {
    this->ContextItemProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (widgetProxy)
    {
      widgetProxy->OnInteraction();
    }
  }
  else if (event == vtkCommand::EndInteractionEvent)
  {
    this->ContextItemProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (widgetProxy)
    {
      widgetProxy->OnEndInteraction();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMNew2DWidgetRepresentationProxy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ContextItem:";
  if (this->ContextItem)
  {
    os << std::endl;
    this->ContextItem->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
  os << indent << "ContextItemProxy:";
  if (this->ContextItemProxy)
  {
    os << std::endl;
    this->ContextItemProxy->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
}
