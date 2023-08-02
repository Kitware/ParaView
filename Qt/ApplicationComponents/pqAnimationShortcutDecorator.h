// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationShortcutDecorator_h
#define pqAnimationShortcutDecorator_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidgetDecorator.h"

class pqAnimationShortcutWidget;
class vtkSMProxy;
class vtkSMProperty;

/**
 * A default decorator to add a pqAnimationShortcutWidget on property widgets
 * from a vtkSMSourceProxy if it is not a vtkSMRepresentationProxy,
 * and if the property is a vector property of a single elements
 * that has a range or a scalar range defined and is animateable.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimationShortcutDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  /**
   * Constructor that modify the widget if conditions are met.
   */
  pqAnimationShortcutDecorator(pqPropertyWidget* parent);
  ~pqAnimationShortcutDecorator() override;

  /**
   * Return true if the widget is considered valid by this decorator
   */
  static bool accept(pqPropertyWidget* widget);

protected Q_SLOTS:
  /**
   * Called when general settings has changed
   * to hide/show the widget if necessary
   */
  virtual void generalSettingsChanged();

private:
  Q_DISABLE_COPY(pqAnimationShortcutDecorator);

  pqAnimationShortcutWidget* Widget;
};

#endif
