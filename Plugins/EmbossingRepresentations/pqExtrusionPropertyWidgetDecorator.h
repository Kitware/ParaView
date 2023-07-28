// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqExtrusionPropertyWidgetDecorator
 * @brief   decorator used to hide/show range inputs for extrusion representation
 *
 * When auto-scaling or normalization is enabled, range inputs must be hidden.
 */

#ifndef pqExtrusionPropertyWidgetDecorator_h
#define pqExtrusionPropertyWidgetDecorator_h

#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

class vtkSMProperty;

class pqExtrusionPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqExtrusionPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  ~pqExtrusionPropertyWidgetDecorator() override;

  bool canShowWidget(bool show_advanced) const override;

private:
  Q_DISABLE_COPY(pqExtrusionPropertyWidgetDecorator)

  vtkWeakPointer<vtkSMProperty> ObservedObject1, ObservedObject2;
  unsigned long ObserverId1, ObserverId2;
};

#endif
