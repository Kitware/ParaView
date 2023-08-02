// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMyPropertyWidgetDecorator_h
#define pqMyPropertyWidgetDecorator_h

#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"
class vtkObject;

class pqMyPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqMyPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  virtual ~pqMyPropertyWidgetDecorator();

  /// Overridden to hide the widget when ShrinkFactor < 0.1
  virtual bool canShowWidget(bool show_advanced) const;

private:
  Q_DISABLE_COPY(pqMyPropertyWidgetDecorator)

  vtkWeakPointer<vtkObject> ObservedObject;
  unsigned long ObserverId;
};

#endif
