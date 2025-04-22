// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEnableWidgetDecorator.h"

//-----------------------------------------------------------------------------
pqEnableWidgetDecorator::pqEnableWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  this->decoratorLogic->Initialize(config, parentObject->proxy());
  this->decoratorLogic->AddObserver(vtkEnableDecorator::EnableStateChangedEvent, this,
    &pqEnableWidgetDecorator::emitEnableStateChanged);
}

//-----------------------------------------------------------------------------
void pqEnableWidgetDecorator::emitEnableStateChanged()
{
  Q_EMIT enableStateChanged();
}
