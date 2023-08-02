// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTimelineScrollbar_h
#define pqTimelineScrollbar_h

#include "pqWidgetsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QWidget>

class QScrollBar;
class QSpacerItem;

class pqAnimationModel;

/**
 * A widget offering a scrollbar useful to interact with the timeline
 * from the animation model.
 */
class PARAVIEW_DEPRECATED_IN_5_11_0(
  "See `pqTimeManagerWidget` for new design.") PQWIDGETS_EXPORT pqTimelineScrollbar : public QWidget
{
  Q_OBJECT

public:
  pqTimelineScrollbar(QWidget* p = nullptr);
  ~pqTimelineScrollbar() override = default;

  /**
   * connects to an existing animation model
   * if the parameter is nullptr, any already existing connection is removed
   */
  void setAnimationModel(pqAnimationModel* model);

  /**
   * connects to an existing spacing constraint notifier
   * if the parameter is nullptr, any already existing connection is removed
   */
  void linkSpacing(QObject* spaceNotifier);

protected Q_SLOTS:

  /**
   * called when the offset of the time scrollbar must be updated
   */
  void updateTimeScrollbar();

  /**
   * called when the time scrollbar must be updated
   */
  void updateTimeScrollbarOffset(int);

  /**
   * called when the time scrollbar is being used in the GUI
   */
  void setTimeZoom(int);

private:
  QScrollBar* TimeScrollBar = nullptr;
  QSpacerItem* ScrollBarSpacer = nullptr;

  QObject* SpacingNotifier = nullptr;

  pqAnimationModel* AnimationModel = nullptr;
};

#endif // pqTimelineScrollbar_h
