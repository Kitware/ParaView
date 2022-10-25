/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNewWidgetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNewWidgetRepresentationProxy.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractWidget.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkSMWidgetRepresentationProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"
#include "vtkWidgetRepresentation.h"

#include <cassert>
#include <list>

vtkStandardNewMacro(vtkSMNewWidgetRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::vtkSMNewWidgetRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::~vtkSMNewWidgetRepresentationProxy()
{
  this->RepresentationProxy = nullptr;
  this->WidgetProxy = nullptr;
  this->Widget = nullptr;
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->RepresentationProxy = this->GetSubProxy("Prop");
  if (!this->RepresentationProxy)
  {
    this->RepresentationProxy = this->GetSubProxy("Prop2D");
  }
  if (!this->RepresentationProxy)
  {
    vtkErrorMacro("A representation proxy must be defined as a Prop (or Prop2D) sub-proxy");
    return;
  }
  this->RepresentationProxy->SetLocation(vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT);

  this->WidgetProxy = this->GetSubProxy("Widget");
  if (this->WidgetProxy)
  {
    this->WidgetProxy->SetLocation(vtkPVSession::CLIENT);
  }

  this->Superclass::CreateVTKObjects();

  // Bind the Widget and the representations on the server side
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetRepresentation"
         << VTKOBJECT(this->RepresentationProxy) << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT);

  // Location 0 is for prototype objects !!! No need to send to the server something.
  if (!this->WidgetProxy || this->Location == 0)
  {
    return;
  }

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->WidgetProxy->GetProperty("Representation"));
  if (pp)
  {
    pp->AddProxy(this->RepresentationProxy);
  }
  this->WidgetProxy->UpdateVTKObjects();

  // Get the local VTK object for that widget
  this->Widget = vtkAbstractWidget::SafeDownCast(this->WidgetProxy->GetClientSideObject());

  if (this->Widget)
  {
    this->Widget->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    this->Widget->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
    this->Widget->AddObserver(vtkCommand::InteractionEvent, this->Observer);
  }

  vtk3DWidgetRepresentation* clientObject =
    vtk3DWidgetRepresentation::SafeDownCast(this->GetClientSideObject());
  clientObject->SetWidget(this->Widget);

  this->SetupPropertiesLinks();
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  this->InvokeEvent(event);

  vtkSMWidgetRepresentationProxy* widgetRepresentation =
    vtkSMWidgetRepresentationProxy::SafeDownCast(this->RepresentationProxy);

  if (event == vtkCommand::StartInteractionEvent)
  {
    this->RepresentationProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (vtkRenderWindowInteractor* iren = this->Widget->GetInteractor())
    {
      iren->InvokeEvent(event);
    }
    if (widgetRepresentation)
    {
      widgetRepresentation->OnStartInteraction();
    }
  }
  else if (event == vtkCommand::InteractionEvent)
  {
    this->RepresentationProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (vtkRenderWindowInteractor* iren = this->Widget->GetInteractor())
    {
      iren->InvokeEvent(event);
    }
    if (widgetRepresentation)
    {
      widgetRepresentation->OnInteraction();
    }
  }
  else if (event == vtkCommand::EndInteractionEvent)
  {
    vtkSMProperty* sizeHandles = this->RepresentationProxy->GetProperty("SizeHandles");
    if (sizeHandles)
    {
      sizeHandles->Modified();
      this->RepresentationProxy->UpdateProperty("SizeHandles");
    }

    this->RepresentationProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    if (widgetRepresentation)
    {
      widgetRepresentation->OnEndInteraction();
    }
    if (vtkRenderWindowInteractor* iren = this->Widget->GetInteractor())
    {
      iren->InvokeEvent(event);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
