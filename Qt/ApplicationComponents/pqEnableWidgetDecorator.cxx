// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEnableWidgetDecorator.h"
#include "pqCoreUtilities.h"

//-----------------------------------------------------------------------------
pqEnableWidgetDecorator::pqEnableWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  this->decoratorLogic->Initialize(config, parentObject->proxy());
  pqCoreUtilities::connect(this->decoratorLogic, vtkEnableDecorator::EnableStateChangedEvent, this,
    SIGNAL(enableStateChanged()));
}
