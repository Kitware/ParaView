// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationTimeToolbar_h
#define pqAnimationTimeToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QPointer>
#include <QToolBar>

class pqAnimationTimeWidget;
class pqAnimationScene;

/**
 * pqAnimationTimeToolbar is a QToolBar containing a pqAnimationTimeWidget.
 * pqAnimationTimeToolbar also ensures that the pqAnimationTimeWidget is
 * tracking the animation scene on the active session.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAnimationTimeToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqAnimationTimeToolbar(const QString& _title, QWidget* _parent = nullptr)
    : Superclass(_title, _parent)
  {
    this->constructor();
  }
  pqAnimationTimeToolbar(QWidget* _parent = nullptr)
    : Superclass(_parent)
  {
    this->constructor();
  }

  /**
   * Provides access to the pqAnimationTimeWidget used.
   */
  pqAnimationTimeWidget* animationTimeWidget() const;
private Q_SLOTS:
  void setAnimationScene(pqAnimationScene* scene);

  /**
   * Update the notation and precision for time display.
   */
  void updateTimeDisplay();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqAnimationTimeToolbar)
  void constructor();
  QPointer<pqAnimationTimeWidget> AnimationTimeWidget;
};

#endif
