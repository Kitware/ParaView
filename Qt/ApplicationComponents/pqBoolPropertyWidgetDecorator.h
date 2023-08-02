// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqBoolPropertyWidgetDecorator_h
#define pqBoolPropertyWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

/**
 * pqBoolPropertyWidgetDecorator is a base class for enable/disable
 * or show/hide widgets based on the status of another property not
 * directly controlled by the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqBoolPropertyWidgetDecorator
  : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqBoolPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqBoolPropertyWidgetDecorator() override;

  bool isBoolProperty() const { return this->BoolProperty; }

Q_SIGNALS:
  void boolPropertyChanged();

protected:
  vtkWeakPointer<vtkSMProperty> Property;
  QString Function;
  int Index;
  unsigned long ObserverId;
  bool BoolProperty;
  QString Value;

private:
  Q_DISABLE_COPY(pqBoolPropertyWidgetDecorator)

  /**
   * updates the enabled state.
   */
  void updateBoolPropertyState();
  /**
   * update this->BoolProperty and fires boolPropertyChanged
   */
  void setBoolProperty(bool val);
};

#endif
