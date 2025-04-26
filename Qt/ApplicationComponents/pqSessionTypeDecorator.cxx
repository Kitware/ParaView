// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSessionTypeDecorator.h"

//-----------------------------------------------------------------------------
pqSessionTypeDecorator::pqSessionTypeDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  auto proxy = parentObject->proxy();
  Q_ASSERT(proxy != nullptr);
  this->decoratorLogic->Initialize(config, proxy);
}

//-----------------------------------------------------------------------------
pqSessionTypeDecorator::~pqSessionTypeDecorator() = default;

//-----------------------------------------------------------------------------
bool pqSessionTypeDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}

//-----------------------------------------------------------------------------
bool pqSessionTypeDecorator::enableWidget() const
{
  return this->decoratorLogic->Enable();
}
