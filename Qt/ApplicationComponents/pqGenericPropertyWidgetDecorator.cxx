// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqGenericPropertyWidgetDecorator.h"
#include "pqCoreUtilities.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqGenericPropertyWidgetDecorator::pqGenericPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  assert(proxy != nullptr);
  this->decoratorLogic->Initialize(config, proxy);
  pqCoreUtilities::connect(this->decoratorLogic,
    vtkGenericPropertyDecorator::VisibilityChangedEvent, this, SIGNAL(visibilityChanged()));
  pqCoreUtilities::connect(this->decoratorLogic,
    vtkGenericPropertyDecorator::EnableStateChangedEvent, this, SIGNAL(enableStateChanged()));
}

//-----------------------------------------------------------------------------
pqGenericPropertyWidgetDecorator::~pqGenericPropertyWidgetDecorator() = default;

//-----------------------------------------------------------------------------
bool pqGenericPropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}

//-----------------------------------------------------------------------------
bool pqGenericPropertyWidgetDecorator::enableWidget() const
{
  return this->decoratorLogic->Enable();
}
