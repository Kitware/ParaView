// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoolPropertyWidgetDecorator.h"
#include "pqCoreUtilities.h"
#include <cassert>

//-----------------------------------------------------------------------------
pqBoolPropertyWidgetDecorator::pqBoolPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  assert(proxy != nullptr);

  this->decoratorLogic->Initialize(config, proxy);

  pqCoreUtilities::connect(this->decoratorLogic, vtkBoolPropertyDecorator::BoolPropertyChangedEvent,
    this, SIGNAL(boolPropertyChanged()));
  this->decoratorLogic->UpdateBoolPropertyState();
}

//-----------------------------------------------------------------------------
bool pqBoolPropertyWidgetDecorator::isBoolProperty() const
{
  return this->decoratorLogic->IsBoolProperty();
}
