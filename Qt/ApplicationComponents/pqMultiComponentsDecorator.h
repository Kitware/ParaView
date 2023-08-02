// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMultiComponentsDecorator_h
#define pqMultiComponentsDecorator_h

#include "pqPropertyWidgetDecorator.h"

#include <vector>

/**
 * pqMultiComponentsDecorator's purpose is to prevent the GUI from
 * showing Multi Components Mapping checkbox when the representation is not Volume,
 * the number of components is not valid or MapScalars is not checked.
 */
class pqMultiComponentsDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqMultiComponentsDecorator(vtkPVXMLElement* config, pqPropertyWidget* parentObject);
  ~pqMultiComponentsDecorator() override = default;

  /**
   * Overridden to hide the widget
   */
  bool canShowWidget(bool show_advanced) const override;

private:
  Q_DISABLE_COPY(pqMultiComponentsDecorator)

  std::vector<int> Components;
};

#endif
