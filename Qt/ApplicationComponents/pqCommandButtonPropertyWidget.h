// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCommandButtonPropertyWidget_h
#define pqCommandButtonPropertyWidget_h

#include "pqApplicationComponentsModule.h"

#include "pqPropertyWidget.h"

/**
 * A property widget with a push button for invoking a command on a proxy.
 *
 * To use this widget for a property add the 'panel_widget="command_button"'
 * to the property's XML.
 *
 * If the property has a "command" attribute set, then the command will be invoked
 * immediately. If the property has no command set, then clicking the button will call
 * `vtkSMProxy::RecreateVTKObjects()`.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCommandButtonPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pqCommandButtonPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqCommandButtonPropertyWidget() override;

protected Q_SLOTS:
  virtual void buttonClicked();

private:
  vtkSMProperty* Property;
};

#endif // pqCommandButtonPropertyWidget_h
