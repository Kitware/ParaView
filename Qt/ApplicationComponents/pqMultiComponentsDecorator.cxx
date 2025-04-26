// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiComponentsDecorator.h"

#include <sstream>

//-----------------------------------------------------------------------------
pqMultiComponentsDecorator::pqMultiComponentsDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = parentObject->proxy();
  this->decoratorLogic->Initialize(config, proxy);
}

//-----------------------------------------------------------------------------
bool pqMultiComponentsDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}
