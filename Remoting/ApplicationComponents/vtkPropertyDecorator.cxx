// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPropertyDecorator.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

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
}

//-----------------------------------------------------------------------------
void vtkPropertyDecorator::Initialize(vtkPVXMLElement* xml_, vtkSMProxy* proxy_)
{
  this->xml = xml_;
  this->proxy = proxy_;
}
//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkPropertyDecorator::XML() const
{
  return this->xml.GetPointer();
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
std::string vtkPropertyDecorator::GetDecoratorType() const
{

  if (this->xml == nullptr || strcmp(this->xml->GetName(), "PropertyDecorator") != 0 ||
    this->xml->GetAttribute("type") == nullptr)
  {
    vtkWarningMacro("Invalid xml config specified. Cannot create a decorator.");
    return "";
  }
  return this->xml->GetAttribute("type");
}
