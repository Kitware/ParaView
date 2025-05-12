// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCompositePropertyWidgetDecorator_h
#define pqCompositePropertyWidgetDecorator_h

#include "pqPropertyWidgetDecorator.h"

#include "vtkCompositePropertyDecorator.h"

/**
 * @class pqCompositePropertyWidgetDecorator
 * @brief pqPropertyWidgetDecorator subclass that can combine multiple
 *        decorators using boolean operations.
 *
 * @see vtkCompositePropertyDecorator
 */
class vtkPropertyDecorator;
class PQCOMPONENTS_EXPORT pqCompositePropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqCompositePropertyWidgetDecorator(vtkPVXMLElement* xml, pqPropertyWidget* parent);
  ~pqCompositePropertyWidgetDecorator() override;

  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;

private:
  Q_DISABLE_COPY(pqCompositePropertyWidgetDecorator)

  vtkNew<vtkCompositePropertyDecorator> decoratorLogic;
};

#endif
