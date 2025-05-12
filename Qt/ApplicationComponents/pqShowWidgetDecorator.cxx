// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqShowWidgetDecorator.h"
#include "pqCoreUtilities.h"

pqShowWidgetDecorator::pqShowWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{

  this->decoratorLogic->Initialize(config, parentObject->proxy());
  pqCoreUtilities::connect(this->decoratorLogic, vtkShowDecorator::VisibilityChangedEvent, this,
    SIGNAL(visibilityChanged()));
}
