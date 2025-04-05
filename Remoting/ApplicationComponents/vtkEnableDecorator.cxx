// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnableDecorator.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEnableDecorator);

//-----------------------------------------------------------------------------
vtkEnableDecorator::vtkEnableDecorator() = default;

//-----------------------------------------------------------------------------
vtkEnableDecorator::~vtkEnableDecorator() = default;

//-----------------------------------------------------------------------------
void vtkEnableDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkEnableDecorator::Initialize(vtkPVXMLElement* xml_, vtkSMProxy* proxy_)
{
  this->Superclass::Initialize(xml_, proxy_);
  this->AddObserver(vtkBoolPropertyDecorator::BoolPropertyChangedEvent, this,
    &vtkEnableDecorator::InvokeEnableStateChangedEvent);
}
