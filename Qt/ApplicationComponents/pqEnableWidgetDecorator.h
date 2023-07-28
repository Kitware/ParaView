// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEnableWidgetDecorator_h
#define pqEnableWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqBoolPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

/**
 * pqEnableWidgetDecorator can be used to enable/disable a widget based on the
 * status of another property not directly controlled by the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEnableWidgetDecorator : public pqBoolPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqBoolPropertyWidgetDecorator Superclass;

public:
  pqEnableWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);

  /**
   * overridden from pqPropertyWidget.
   */
  bool enableWidget() const override { return this->isBoolProperty(); }

private:
  Q_DISABLE_COPY(pqEnableWidgetDecorator)
};

#endif
