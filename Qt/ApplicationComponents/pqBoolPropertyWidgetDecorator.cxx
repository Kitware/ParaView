// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoolPropertyWidgetDecorator.h"
#include <cassert>

//-----------------------------------------------------------------------------
pqBoolPropertyWidgetDecorator::pqBoolPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
  , ObserverId(0)
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  assert(proxy != nullptr);

  this->decoratorLogic->Initialize(config, proxy);

  this->ObserverId =
    this->decoratorLogic->AddObserver(vtkBoolPropertyDecorator::BoolPropertyChangedEvent, this,
      &pqBoolPropertyWidgetDecorator::emitUpdateBoolPropertyState);
  this->decoratorLogic->UpdateBoolPropertyState();
}

//-----------------------------------------------------------------------------
pqBoolPropertyWidgetDecorator::~pqBoolPropertyWidgetDecorator()
{
  if (this->ObserverId)
  {
    this->decoratorLogic->RemoveObserver(this->ObserverId);
  }
}
//-----------------------------------------------------------------------------
bool pqBoolPropertyWidgetDecorator::isBoolProperty() const
{
  return this->decoratorLogic->IsBoolProperty();
}
//-----------------------------------------------------------------------------
void pqBoolPropertyWidgetDecorator::emitUpdateBoolPropertyState()
{
  Q_EMIT this->boolPropertyChanged();
}
