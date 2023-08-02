// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCommandPropertyWidget_h
#define pqCommandPropertyWidget_h

#include "pqPropertyWidget.h"

/**
 * pqCommandPropertyWidget is used for vtkSMProperty instances (not one of its
 * subclasses). It simply creates a button that the users can press. Unlike
 * other pqPropertyWidget subclasses, the result of clicking this button does
 * not affect the state of the Apply/Reset buttons. It triggers the action
 * prompted by the property immediately.
 */
class PQCOMPONENTS_EXPORT pqCommandPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqCommandPropertyWidget(vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqCommandPropertyWidget() override;

protected Q_SLOTS:
  /**
   * called when the button is clicked by the user.
   */
  virtual void buttonClicked();

private:
  Q_DISABLE_COPY(pqCommandPropertyWidget)
};

#endif
