// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include <pqOSPRayHidingDecorator.h>
//-----------------------------------------------------------------------------
pqOSPRayHidingDecorator::pqOSPRayHidingDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  this->decoratorLogic->Initialize(config, parentObject->proxy());
}

//-----------------------------------------------------------------------------
pqOSPRayHidingDecorator::~pqOSPRayHidingDecorator() = default;

//-----------------------------------------------------------------------------
bool pqOSPRayHidingDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}
