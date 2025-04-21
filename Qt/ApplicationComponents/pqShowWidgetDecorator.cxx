// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqShowWidgetDecorator.h"

//-----------------------------------------------------------------------------
pqShowWidgetDecorator::pqShowWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{

  this->decoratorLogic->Initialize(config, parentObject->proxy());
  this->decoratorLogic->AddObserver(
    vtkShowDecorator::VisibilityChangedEvent, this, &pqShowWidgetDecorator::emitVisibilityChanged);
}

void pqShowWidgetDecorator::emitVisibilityChanged()
{
  Q_EMIT visibilityChanged();
}
