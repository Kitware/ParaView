// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqANARIHidingDecorator_h
#define pqANARIHidingDecorator_h

#include "pqPropertyWidgetDecorator.h"

#include "vtkANARIHidingDecorator.h"

/**
 * pqANARIHidingDecorator's purpose is to prevent the GUI from
 * showing any of the ANARI specific rendering controls when
 * Paraview is not configured with PARAVIEW_ENABLE_ANARI
 *
 * @see vtkANARIHidingDecorator
 */
class pqANARIHidingDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqANARIHidingDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  ~pqANARIHidingDecorator() override = default;

  /**
   * Overridden to hide the widget when ANARI is not compiled in
   */
  bool canShowWidget(bool showAdvanced) const override;

private:
  Q_DISABLE_COPY(pqANARIHidingDecorator)
  vtkNew<vtkANARIHidingDecorator> decoratorLogic;
};

#endif
