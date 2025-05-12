// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInputDataTypeDecorator_h
#define pqInputDataTypeDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkInputDataTypeDecorator.h"

/**
 * pqInputDataTypeDecorator is a pqPropertyWidgetDecorator subclass.
 * For certain properties, they should update the enable state
 * based on input data types.
 * For example, "Computer Gradients" in Contour filter should only
 * be enabled when an input data type is a StructuredData. Please see
 * vtkPVDataInformation::IsDataStructured() for structured types.
 *
 * @see vtkInputDataTypeDecorator
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqInputDataTypeDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqInputDataTypeDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqInputDataTypeDecorator() override;

  /**
   * Overridden to enable/disable the widget based on input data type.
   */
  bool enableWidget() const override;

  /**
   * Overriden to show or not the widget based on input data type.
   */
  bool canShowWidget(bool show_advanced) const override;

private:
  Q_DISABLE_COPY(pqInputDataTypeDecorator)

  vtkNew<vtkInputDataTypeDecorator> decoratorLogic;
};

#endif
