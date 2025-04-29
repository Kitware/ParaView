// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqGenericPropertyWidgetDecorator_h
#define pqGenericPropertyWidgetDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

#include "vtkGenericPropertyDecorator.h"

/**
 * pqGenericPropertyWidgetDecorator is a pqPropertyWidgetDecorator that
 * supports multiple common use cases from a pqPropertyWidgetDecorator.
 * For a complete list see @see vtkGenericPropertyDecorator.
 *
 * @see vtkGenericPropertyDecorator
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqGenericPropertyWidgetDecorator
  : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqGenericPropertyWidgetDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqGenericPropertyWidgetDecorator() override;

  /**
   * Methods overridden from pqPropertyWidget.
   */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqGenericPropertyWidgetDecorator)
  vtkNew<vtkGenericPropertyDecorator> decoratorLogic;
};

#endif
