// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationShortcutWidget_h
#define pqAnimationShortcutWidget_h

#include "pqApplicationComponentsModule.h"
#include <QToolButton>

class vtkSMProxy;
class vtkSMProperty;
class pqLineEdit;
class pqAnimationScene;

/**
 * A QToolButton widget to show a dialog that is a shortcut to creating an animation
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimationShortcutWidget : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  /**
   * constructor requires the proxy and property
   */
  pqAnimationShortcutWidget(QWidget* parent, vtkSMProxy* proxy, vtkSMProperty* property);
  ~pqAnimationShortcutWidget() override;

protected Q_SLOTS:
  /**
   * Called when the menu is about to be shown.
   */
  virtual void updateMenu();

  /**
   * Called when toolbutton it pressed
   */
  virtual void onTriggered(QAction*);

  /**
   * Set the scene to view
   */
  virtual void setScene(pqAnimationScene* scene);

protected: // NOLINT(readability-redundant-access-specifiers)
  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  pqAnimationScene* Scene;
};

#endif
