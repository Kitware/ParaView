// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPropertyWidgetInterface.h"

//-----------------------------------------------------------------------------
pqPropertyWidgetInterface::~pqPropertyWidgetInterface() = default;

//-----------------------------------------------------------------------------
pqPropertyWidget* pqPropertyWidgetInterface::createWidgetForProperty(
  vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parentWidget)
{
  Q_UNUSED(proxy);
  Q_UNUSED(property);
  Q_UNUSED(parentWidget);
  return nullptr;
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqPropertyWidgetInterface::createWidgetForPropertyGroup(
  vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parentWidget)
{
  Q_UNUSED(proxy);
  Q_UNUSED(group);
  Q_UNUSED(parentWidget);
  return nullptr;
}

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator* pqPropertyWidgetInterface::createWidgetDecorator(
  const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget)
{
  Q_UNUSED(type);
  Q_UNUSED(config);
  Q_UNUSED(widget);
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqPropertyWidgetInterface::createDefaultWidgetDecorators(pqPropertyWidget* widget)
{
  Q_UNUSED(widget);
}
