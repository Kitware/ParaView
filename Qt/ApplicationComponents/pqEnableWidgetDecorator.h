// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEnableWidgetDecorator_h
#define pqEnableWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqBoolPropertyWidgetDecorator.h"
#include "vtkEnableDecorator.h"

/**
 * pqEnableWidgetDecorator can be used to enable/disable a widget based on the
 * status of another property not directly controlled by the widget.
 *
 * @see vtkEnableDecorator
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEnableWidgetDecorator : public pqBoolPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqBoolPropertyWidgetDecorator Superclass;

public:
  pqEnableWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqEnableWidgetDecorator() override = default;

  /**
   * overridden from pqPropertyWidget.
   */
  bool enableWidget() const override { return this->decoratorLogic->IsBoolProperty(); }

private:
  Q_DISABLE_COPY(pqEnableWidgetDecorator)

  vtkNew<vtkEnableDecorator> decoratorLogic;
};

#endif
