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

struct vtkSMNew2DWidgetRepresentationProxy::Internals
{
  typedef std::list<vtkSmartPointer<vtkSMLink>> LinksType;
  LinksType Links;

  vtkWeakPointer<vtkContextItem> ContextItem;

  // Data about "controlled proxies".
  vtkWeakPointer<vtkSMProxy> ControlledProxy;
  vtkWeakPointer<vtkSMPropertyGroup> ControlledPropertyGroup;
};

class vtkSMNew2DWidgetRepresentationObserver : public vtkCommand
{
public:
  static vtkSMNew2DWidgetRepresentationObserver* New()
  {
    return new vtkSMNew2DWidgetRepresentationObserver;
  }
  void Execute(vtkObject* caller, unsigned long event, void*) override
  {
    if (this->Proxy)
    {
      if (vtkContextItem::SafeDownCast(caller))
      {
        this->Proxy->ExecuteEvent(event);
      }
      else if (vtkSMProperty* prop = vtkSMProperty::SafeDownCast(caller))
      {
        this->Proxy->ProcessLinkedPropertyEvent(prop, event);
      }
    }
  }
  vtkSMNew2DWidgetRepresentationObserver()
    : Proxy(nullptr)
  {
  }
  vtkSMNew2DWidgetRepresentationProxy* Proxy;
};

vtkStandardNewMacro(vtkSMNew2DWidgetRepresentationProxy);

vtkSMNew2DWidgetRepresentationProxy::vtkSMNew2DWidgetRepresentationProxy()
  : vtkSMProxy()
  , ContextItemProxy(nullptr)
  , Observer(new vtkSMNew2DWidgetRepresentationObserver())
  , Internal(new Internals())
{
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
  this->Observer->Proxy = this;
}

vtkSMNew2DWidgetRepresentationProxy::~vtkSMNew2DWidgetRepresentationProxy()
{
  this->ContextItemProxy = nullptr;
  this->Observer->Delete();
  if (this->Internal)
  {
    delete this->Internal;
  }
}

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
  // Bind the Widget and the representations on the server side
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetContextItem"
         << VTKOBJECT(this->ContextItemProxy) << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER | vtkPVSession::CLIENT);

  vtk2DWidgetRepresentation* clientObject =
    vtk2DWidgetRepresentation::SafeDownCast(this->GetClientSideObject());

  this->Internal->ContextItem = clientObject->GetContextItem();
  this->Internal->ContextItem->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
  this->Internal->ContextItem->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
  this->Internal->ContextItem->AddObserver(vtkCommand::InteractionEvent, this->Observer);

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

void vtkSMNew2DWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  this->InvokeEvent(event);
}

void vtkSMNew2DWidgetRepresentationProxy::ProcessLinkedPropertyEvent(
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

void vtkSMNew2DWidgetRepresentationProxy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkSMNew2DWidgetRepresentationProxy::LinkProperties(
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

bool vtkSMNew2DWidgetRepresentationProxy::UnlinkProperties(vtkSMProxy* controlledProxy)
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
