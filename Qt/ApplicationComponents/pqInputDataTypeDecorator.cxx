// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqInputDataTypeDecorator.h"
#include "pqCoreUtilities.h"

//-----------------------------------------------------------------------------
pqInputDataTypeDecorator::pqInputDataTypeDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = parentObject->proxy();
  this->decoratorLogic->Initialize(config, proxy);
  pqCoreUtilities::connect(this->decoratorLogic, vtkPropertyDecorator::EnableStateChangedEvent,
    this, SIGNAL(enableStateChanged()));
}

//-----------------------------------------------------------------------------
pqInputDataTypeDecorator::~pqInputDataTypeDecorator() = default;

//-----------------------------------------------------------------------------
bool pqInputDataTypeDecorator::enableWidget() const
{
  return this->decoratorLogic->Enable();
}

//-----------------------------------------------------------------------------
bool pqInputDataTypeDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}
