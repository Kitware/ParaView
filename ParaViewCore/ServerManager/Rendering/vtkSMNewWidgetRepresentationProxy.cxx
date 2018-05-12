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

class vtkSMNewWidgetRepresentationObserver : public vtkCommand
{
public:
  static vtkSMNewWidgetRepresentationObserver* New()
  {
    return new vtkSMNewWidgetRepresentationObserver;
  }
  void Execute(vtkObject* caller, unsigned long event, void*) override
  {
    if (this->Proxy)
    {
      if (vtkAbstractWidget::SafeDownCast(caller))
      {
        this->Proxy->ExecuteEvent(event);
      }
      else if (vtkSMProperty* prop = vtkSMProperty::SafeDownCast(caller))
      {
        this->Proxy->ProcessLinkedPropertyEvent(prop, event);
      }
    }
  }
  vtkSMNewWidgetRepresentationObserver()
    : Proxy(0)
  {
  }
  vtkSMNewWidgetRepresentationProxy* Proxy;
};

struct vtkSMNewWidgetRepresentationInternals
{
  typedef std::list<vtkSmartPointer<vtkSMLink> > LinksType;
  LinksType Links;
  vtkWeakPointer<vtkSMRenderViewProxy> ViewProxy;

  // Data about "controlled proxies".
  vtkWeakPointer<vtkSMProxy> ControlledProxy;
  vtkWeakPointer<vtkSMPropertyGroup> ControlledPropertyGroup;
};

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::vtkSMNewWidgetRepresentationProxy()
{
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer = vtkSMNewWidgetRepresentationObserver::New();
  this->Observer->Proxy = this;
  this->Internal = new vtkSMNewWidgetRepresentationInternals;
}

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::~vtkSMNewWidgetRepresentationProxy()
{
  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer->Proxy = 0;
  this->Observer->Delete();

  if (this->Internal)
  {
    delete this->Internal;
  }
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

  // Since links copy values from input to output,
  // we need to make sure that input properties i.e. the info
  // properties are not empty.
  this->UpdatePropertyInformation();

  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
  {
    vtkSMProperty* prop = piter->GetProperty();
    vtkSMProperty* info = prop->GetInformationProperty();
    if (info)
    {
      // This ensures that the property value from the loaded state is
      // preserved, and not overwritten by the default value from
      // the property information
      info->Copy(prop);

      vtkSMPropertyLink* link = vtkSMPropertyLink::New();

      // NOTE: vtkSMPropertyLink no longer affect proxy reference. We're now
      // only using vtkWeakPointer in vtkSMPropertyLink.
      link->AddLinkedProperty(this, piter->GetKey(), vtkSMLink::OUTPUT);
      link->AddLinkedProperty(this, this->GetPropertyName(info), vtkSMLink::INPUT);

      this->Internal->Links.push_back(link);
      link->Delete();
    }
  }
  piter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  this->InvokeEvent(event);

  vtkSMWidgetRepresentationProxy* widgetRepresentation =
    vtkSMWidgetRepresentationProxy::SafeDownCast(this->RepresentationProxy);

  if (event == vtkCommand::StartInteractionEvent)
  {
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
bool vtkSMNewWidgetRepresentationProxy::LinkProperties(
  vtkSMProxy* controlledProxy, vtkSMPropertyGroup* controlledPropertyGroup)
{
  if (this->Internal->ControlledProxy != NULL)
  {
    vtkErrorMacro("Cannot `LinkProperties` with multiple proxies.");
    return false;
  }

  this->Internal->ControlledProxy = controlledProxy;
  this->Internal->ControlledPropertyGroup = controlledPropertyGroup;
  for (unsigned int cc = 0, max = controlledPropertyGroup->GetNumberOfProperties(); cc < max; ++cc)
  {
    vtkSMProperty* prop = controlledPropertyGroup->GetProperty(cc);
    const char* function = controlledPropertyGroup->GetFunction(prop);
    if (vtkSMProperty* widgetProperty = this->GetProperty(function))
    {
      prop->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, this->Observer);
      widgetProperty->AddObserver(vtkCommand::ModifiedEvent, this->Observer);

      vtkSMUncheckedPropertyHelper helper(prop);
      vtkSMPropertyHelper(widgetProperty).Copy(helper);
    }
  }
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMNewWidgetRepresentationProxy::UnlinkProperties(vtkSMProxy* controlledProxy)
{
  if (this->Internal->ControlledProxy != controlledProxy)
  {
    vtkErrorMacro("Cannot 'UnlinkProperties' from a non-linked proxy.");
    return false;
  }

  vtkSMPropertyGroup* controlledPropertyGroup = this->Internal->ControlledPropertyGroup;
  for (unsigned int cc = 0, max = controlledPropertyGroup->GetNumberOfProperties(); cc < max; ++cc)
  {
    vtkSMProperty* prop = controlledPropertyGroup->GetProperty(cc);
    prop->RemoveObserver(this->Observer);
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::ProcessLinkedPropertyEvent(
  vtkSMProperty* caller, unsigned long event)
{
  assert(this->Internal->ControlledPropertyGroup);
  vtkSMPropertyGroup* controlledPropertyGroup = this->Internal->ControlledPropertyGroup;
  if (event == vtkCommand::UncheckedPropertyModifiedEvent)
  {
    // Whenever a controlled property's unchecked value changes, we copy that
    // value
    vtkSMProperty* controlledProperty = caller;
    const char* function = controlledPropertyGroup->GetFunction(controlledProperty);
    if (vtkSMProperty* widgetProperty = this->GetProperty(function))
    {
      // Copy unchecked values from controlledProperty to the checked values of
      // this widget's property.
      vtkSMUncheckedPropertyHelper chelper(controlledProperty);
      vtkSMPropertyHelper(widgetProperty).Copy(chelper);
      this->UpdateVTKObjects();
    }
  }
  else if (event == vtkCommand::ModifiedEvent)
  {
    // Whenever a property on the widget is modified, we change the unchecked
    // property on the linked controlled proxy.
    vtkSMProperty* widgetProperty = caller;
    const char* function = this->GetPropertyName(widgetProperty);
    if (vtkSMProperty* controlledProperty = controlledPropertyGroup->GetProperty(function))
    {
      vtkSMPropertyHelper whelper(widgetProperty);
      vtkSMUncheckedPropertyHelper(controlledProperty).Copy(whelper);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
