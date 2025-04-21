// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkShowDecorator.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkShowDecorator);

//-----------------------------------------------------------------------------
vtkShowDecorator::vtkShowDecorator() = default;

//-----------------------------------------------------------------------------
vtkShowDecorator::~vtkShowDecorator() = default;

//-----------------------------------------------------------------------------
void vtkShowDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkShowDecorator::Initialize(vtkPVXMLElement* xml_, vtkSMProxy* proxy_)
{
  this->Superclass::Initialize(xml_, proxy_);
  this->AddObserver(vtkBoolPropertyDecorator::BoolPropertyChangedEvent, this,
    &vtkShowDecorator::InvokeVisibilityChangedEvent);
}
