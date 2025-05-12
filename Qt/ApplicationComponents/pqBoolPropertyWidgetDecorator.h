// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqBoolPropertyWidgetDecorator_h
#define pqBoolPropertyWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkBoolPropertyDecorator.h"

/**
 * pqBoolPropertyWidgetDecorator is a base class for enable/disable
 * or show/hide widgets based on the status of another property not
 * directly controlled by the widget.
 *
 * @see vtkBoolPropertyDecorator
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqBoolPropertyWidgetDecorator
  : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqBoolPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqBoolPropertyWidgetDecorator() override = default;

  bool isBoolProperty() const;

Q_SIGNALS:
  void boolPropertyChanged();

private:
  Q_DISABLE_COPY(pqBoolPropertyWidgetDecorator)

  vtkNew<vtkBoolPropertyDecorator> decoratorLogic;
};

#endif
