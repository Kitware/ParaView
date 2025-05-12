// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPropertyDecorator.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

#include "vtkCompositePropertyDecorator.h"
#include "vtkEnableDecorator.h"
#include "vtkGenericPropertyDecorator.h"
#include "vtkInputDataTypeDecorator.h"
#include "vtkMultiComponentsDecorator.h"
#include "vtkOSPRayHidingDecorator.h"
#include "vtkSessionTypeDecorator.h"
#include "vtkShowDecorator.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPropertyDecorator);

//-----------------------------------------------------------------------------
vtkPropertyDecorator::vtkPropertyDecorator() = default;

//-----------------------------------------------------------------------------
vtkPropertyDecorator::~vtkPropertyDecorator() = default;

//-----------------------------------------------------------------------------
void vtkPropertyDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Proxy: " << this->proxy_ << std::endl;
  os << indent << "XML: " << this->xml_ << std::endl;
  if (this->xml_)
  {
    this->xml_->PrintXML(os, indent);
  }
}

//-----------------------------------------------------------------------------
void vtkPropertyDecorator::Initialize(vtkPVXMLElement* xml, vtkSMProxy* proxy)
{
  this->xml_ = xml;
  this->proxy_ = proxy;
}
//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkPropertyDecorator::XML() const
{
  return this->xml_.GetPointer();
}
//-----------------------------------------------------------------------------
vtkSMProxy* vtkPropertyDecorator::Proxy() const
{
  return this->proxy_.GetPointer();
}
//-----------------------------------------------------------------------------
void vtkPropertyDecorator::InvokeVisibilityChangedEvent()
{
  this->InvokeEvent(VisibilityChangedEvent);
}

//-----------------------------------------------------------------------------
void vtkPropertyDecorator::InvokeEnableStateChangedEvent()
{
  this->InvokeEvent(EnableStateChangedEvent);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPropertyDecorator> vtkPropertyDecorator::Create(
  vtkPVXMLElement* xmlconfig, vtkSMProxy* proxy)
{
  if (xmlconfig == nullptr || strcmp(xmlconfig->GetName(), "PropertyWidgetDecorator") != 0 ||
    xmlconfig->GetAttribute("type") == nullptr)
  {
    // vtkGenericWarningMacro("Invalid xml config specified. Cannot create a decorator.");
    return nullptr;
  }

  vtkSmartPointer<vtkPropertyDecorator> decorator;
  const std::string type = xmlconfig->GetAttribute("type");
  // *** NOTE: When adding new types, please update the header documentation ***
  if (type == "InputDataTypeDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkInputDataTypeDecorator::New());
  }
  if (type == "EnableWidgetDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkEnableDecorator::New());
  }
  if (type == "ShowWidgetDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkShowDecorator::New());
  }
  if (type == "GenericDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkGenericPropertyDecorator::New());
  }
  if (type == "OSPRayHidingDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkOSPRayHidingDecorator::New());
  }
  if (type == "MultiComponentsDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkMultiComponentsDecorator::New());
  }
  if (type == "CompositeDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkCompositePropertyDecorator::New());
  }
  if (type == "SessionTypeDecorator")
  {
    decorator = vtk::TakeSmartPointer(vtkSessionTypeDecorator::New());
  }

  if (decorator)
  {
    decorator->Initialize(xmlconfig, proxy);
  }

  // *** NOTE: When adding new types, please update the header documentation ***
  return decorator;
}
