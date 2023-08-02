// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimeManagerWidget_h
#define pqTimeManagerWidget_h

#include "pqApplicationComponentsModule.h"

#include <QWidget>

#include <memory> // for unique_ptr

class pqAnimationScene;

/**
 * pqTimeManagerWidget is the main widget for the Time Manager dock.
 *
 * It contains widgets to control current time and animation.
 * This is the main graphical interface where to:
 *  - setup scene time including stride
 *  - select temporal sources that contributes to available time
 *  - create and edit animation tracks
 *
 * @sa pqTimelineWidget, pqTimelineView and pqTimelineModel.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimeManagerWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqTimeManagerWidget(QWidget* parent = nullptr);
  ~pqTimeManagerWidget() override;

protected Q_SLOTS:
  void updateWidgetsVisibility();

  /**
   * When settings changed, we need to redraw the widget
   * to adapt notation and precision.
   */
  void onSettingsChanged();

  /**
   * Set active scene. Updates some connections.
   */
  void setActiveScene(pqAnimationScene*);

private:
  Q_DISABLE_COPY(pqTimeManagerWidget)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
