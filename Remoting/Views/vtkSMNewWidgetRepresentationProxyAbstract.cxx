/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNewWidgetRepresentationProxyAbstract.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNewWidgetRepresentationProxyAbstract.h"
#include "vtkObjectFactory.h"

#include "vtkAbstractWidget.h"
#include "vtkCommand.h"
#include "vtkContextItem.h"
#include "vtkPVSession.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxyAbstract::vtkSMWidgetObserver::vtkSMWidgetObserver() = default;

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxyAbstract::vtkSMWidgetObserver::Execute(
  vtkObject* caller, unsigned long event, void* vtkNotUsed(userData))
{
  if (this->WidgetRepresentation)
  {
    if (vtkAbstractWidget::SafeDownCast(caller) || vtkContextItem::SafeDownCast(caller))
    {
      this->WidgetRepresentation->ExecuteEvent(event);
    }
    else if (vtkSMProperty* prop = vtkSMProperty::SafeDownCast(caller))
    {
      this->WidgetRepresentation->ProcessLinkedPropertyEvent(prop, event);
    }
  }
}

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxyAbstract::vtkSMNewWidgetRepresentationProxyAbstract()
{
  this->Observer->WidgetRepresentation = this;
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxyAbstract::~vtkSMNewWidgetRepresentationProxyAbstract() = default;

//----------------------------------------------------------------------------
bool vtkSMNewWidgetRepresentationProxyAbstract::LinkProperties(
  vtkSMProxy* controlledProxy, vtkSMPropertyGroup* controlledPropertyGroup)
{
  if (this->ControlledProxy != nullptr)
  {
    vtkErrorMacro("Cannot `LinkProperties` with multiple proxies.");
    return false;
  }

  this->ControlledProxy = controlledProxy;
  this->ControlledPropertyGroup = controlledPropertyGroup;
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
bool vtkSMNewWidgetRepresentationProxyAbstract::UnlinkProperties(vtkSMProxy* controlledProxy)
{
  if (this->ControlledProxy != controlledProxy)
  {
    vtkErrorMacro("Cannot 'UnlinkProperties' from a non-linked proxy.");
    return false;
  }

  vtkSMPropertyGroup* controlledPropertyGroup = this->ControlledPropertyGroup;
  for (unsigned int cc = 0, max = controlledPropertyGroup->GetNumberOfProperties(); cc < max; ++cc)
  {
    vtkSMProperty* prop = controlledPropertyGroup->GetProperty(cc);
    prop->RemoveObserver(this->Observer);
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxyAbstract::SetupPropertiesLinks()
{
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

      this->Links.push_back(link);
      link->Delete();
    }
  }
  piter->Delete();
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxyAbstract::ProcessLinkedPropertyEvent(
  vtkSMProperty* caller, unsigned long event)
{
  assert(this->ControlledPropertyGroup);
  vtkSMPropertyGroup* controlledPropertyGroup = this->ControlledPropertyGroup;
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
void vtkSMNewWidgetRepresentationProxyAbstract::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ControlledProxy:";
  if (this->ControlledProxy)
  {
    os << std::endl;
    this->ControlledProxy->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
  os << indent << "ControlledPropertyGroup:";
  if (this->ControlledPropertyGroup)
  {
    os << std::endl;
    this->ControlledPropertyGroup->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
  os << indent << "Links: size " << this->Links.size() << std::endl;
}
