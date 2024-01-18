// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqArrayStatusPropertyWidget_h
#define pqArrayStatusPropertyWidget_h

#include "pqPropertyWidget.h"
#include <QScopedPointer>

class vtkObject;
class vtkPVXMLElement;
class vtkSMPropertyGroup;

class PQCOMPONENTS_EXPORT pqArrayStatusPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqArrayStatusPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parent = nullptr);
  pqArrayStatusPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqArrayStatusPropertyWidget() override;

private Q_SLOTS:
  void updateColumn(vtkObject*);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqArrayStatusPropertyWidget);
  class pqInternals;
  friend class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
