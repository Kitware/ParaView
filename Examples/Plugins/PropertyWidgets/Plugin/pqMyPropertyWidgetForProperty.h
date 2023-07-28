// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMyPropertyWidgetForProperty_h
#define pqMyPropertyWidgetForProperty_h

#include "pqPropertyWidget.h"

class pqMyPropertyWidgetForProperty : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqMyPropertyWidgetForProperty(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0);
  virtual ~pqMyPropertyWidgetForProperty();

private:
  Q_DISABLE_COPY(pqMyPropertyWidgetForProperty)
};

#endif
