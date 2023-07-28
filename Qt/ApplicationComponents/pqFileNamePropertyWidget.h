// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFileNamePropertyWidget_h
#define pqFileNamePropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * pqFileNamePropertyWidget is used for the "File Name" property on
 * the Environment annotation filter. It customizes the resetButtonClicked()
 * logic since the Environment filter's filename setup is custom.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqFileNamePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqFileNamePropertyWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqFileNamePropertyWidget() override;

Q_SIGNALS:
  /**
   * internal signal used to clear highlights from pqHighlightablePushButton.
   */
  void clearHighlight();
  void highlightResetButton();

protected Q_SLOTS:
  /**
   * update the property's value using the domain.
   */
  virtual void resetButtonClicked();

private:
  Q_DISABLE_COPY(pqFileNamePropertyWidget)
};

#endif
