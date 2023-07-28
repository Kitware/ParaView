// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqShowWidgetDecorator_h
#define pqShowWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqBoolPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

/**
 * pqShowWidgetDecorator can be used to show/hide a widget based on the
 * status of another property not directly controlled by the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqShowWidgetDecorator : public pqBoolPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqBoolPropertyWidgetDecorator Superclass;

public:
  pqShowWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);

  bool canShowWidget(bool show_advanced) const override
  {
    (void)show_advanced;
    return this->isBoolProperty();
  }

private:
  Q_DISABLE_COPY(pqShowWidgetDecorator)
};

#endif
