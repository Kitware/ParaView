// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomViewpointsToolbar_h
#define pqCustomViewpointsToolbar_h

#include "pqApplicationComponentsModule.h"

#include <QPixmap>
#include <QPointer>
#include <QToolBar>

/**
 * pqCustomViewpointsToolbar is the toolbar that has buttons for using and configuring
 * custom views (aka camera positions)
 */
class QAction;
class pqCustomViewpointsController;

class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqCustomViewpointsToolbar(
    const QString& title, pqCustomViewpointsController* controller, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
    , Controller(controller)
    , BasePixmap(64, 64)
  {
    this->constructor();
  }
  pqCustomViewpointsToolbar(
    pqCustomViewpointsController* controller, QWidget* parentObject = nullptr)
    : Superclass(parentObject)
    , Controller(controller)
    , BasePixmap(64, 64)
  {
    this->constructor();
  }
  pqCustomViewpointsToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
    , Controller(nullptr)
    , BasePixmap(64, 64)
  {
    this->constructor();
  }
  ~pqCustomViewpointsToolbar() override = default;

  /**
   * Clear and recreate all custom viewpoint actions
   * based on current settings
   */
  void updateCustomViewpointActions();

protected Q_SLOTS:
  /**
   * Update the state of the toolbuttons
   * depending of the type of the current active view
   */
  void updateEnabledState();

  /**
   * Open the Custom Viewpoint
   * button dialog to configure the viewpoints
   * manually
   */
  void configureCustomViewpoints();

  /**
   * Slot to apply a custom view point
   */
  void applyCustomViewpoint();

  /**
   * Slot to add current viewpoint
   * to a new custom viewpoint
   */
  void addCurrentViewpointToCustomViewpoints();

  /**
   * Slot to set a custom viewpoint
   * to a current viewpoint
   */
  void setToCurrentViewpoint();

  /**
   * Slot to delete a custom view point
   */
  void deleteCustomViewpoint();

private:
  Q_DISABLE_COPY(pqCustomViewpointsToolbar)
  void constructor();

  pqCustomViewpointsController* Controller;
  QPointer<QAction> PlusAction;
  QPointer<QAction> ConfigAction;
  QPixmap BasePixmap;
  QPixmap PlusPixmap;
  QPixmap ConfigPixmap;
  QVector<QPointer<QAction>> ViewpointActions;
};

#endif
