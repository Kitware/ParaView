// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMyPropertyWidgetForGroup_h
#define pqMyPropertyWidgetForGroup_h

#include "pqPropertyWidget.h"

class vtkSMPropertyGroup;

class pqMyPropertyWidgetForGroup : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqMyPropertyWidgetForGroup(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = 0);
  virtual ~pqMyPropertyWidgetForGroup();

private:
  Q_DISABLE_COPY(pqMyPropertyWidgetForGroup)
};

#endif
