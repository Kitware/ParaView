// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSessionTypeDecorator_h
#define pqSessionTypeDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"
#include <QObject>

#include "vtkSessionTypeDecorator.h"

/**
 * @class pqSessionTypeDecorator
 * @brief decorator to show/hide or enable/disable property widget based on the
 *        session.
 *
 * pqSessionTypeDecorator is a pqPropertyWidgetDecorator subclass that can be
 * used to show/hide or enable/disable a pqPropertyWidget based on the current
 * session.
 *
 * @see vtkSessionTypeDecorator
 *
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSessionTypeDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  pqSessionTypeDecorator(vtkPVXMLElement* config, pqPropertyWidget* parent);
  ~pqSessionTypeDecorator() override;

  ///@{
  /**
   * Methods overridden from pqPropertyWidget.
   */
  bool canShowWidget(bool show_advanced) const override;
  bool enableWidget() const override;
  ///@}

private:
  Q_DISABLE_COPY(pqSessionTypeDecorator);

  vtkNew<vtkSessionTypeDecorator> decoratorLogic;
};

#endif
