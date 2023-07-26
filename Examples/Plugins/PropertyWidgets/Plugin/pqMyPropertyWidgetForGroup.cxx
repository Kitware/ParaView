// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqMyPropertyWidgetForGroup.h"

//-----------------------------------------------------------------------------
pqMyPropertyWidgetForGroup::pqMyPropertyWidgetForGroup(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  Q_UNUSED(smgroup);
}

//-----------------------------------------------------------------------------
pqMyPropertyWidgetForGroup::~pqMyPropertyWidgetForGroup() = default;
