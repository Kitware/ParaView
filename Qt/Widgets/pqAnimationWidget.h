// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqAnimationWidget_h
#define pqAnimationWidget_h

#include "pqWidgetsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QAbstractScrollArea>
#include <QStandardItemModel>

class QGraphicsView;
class QHeaderView;
class pqAnimationModel;
class pqAnimationTrack;

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "See `pqTimeManagerWidget` for new design.") PQWIDGETS_EXPORT pqAnimationWidget
  : public QAbstractScrollArea
{
  Q_OBJECT;
  using Superclass = QAbstractScrollArea;

public:
  pqAnimationWidget(QWidget* p = nullptr);
  ~pqAnimationWidget() override = default;

  pqAnimationModel* animationModel() const;

  /**
   * Enabled header is used to show if the track is enabled.
   */
  QHeaderView* enabledHeader() const;

  QHeaderView* createDeleteHeader() const;
  QWidget* createDeleteWidget() const;

Q_SIGNALS:
  // emitted when a track is double clicked on
  void trackSelected(pqAnimationTrack*);
  void deleteTrackClicked(pqAnimationTrack*);
  void createTrackClicked();
  // emitted when the timeline offset is changed
  void timelineOffsetChanged(int);

  /**
   * request enable/disabling of the track.
   */
  void enableTrackClicked(pqAnimationTrack*);

protected Q_SLOTS:
  void updateSizes();
  void headerDblClicked(int);
  void headerDeleteClicked(int);
  void headerEnabledClicked(int which);

protected: // NOLINT(readability-redundant-access-specifiers)
  void updateGeometries();
  void updateScrollBars();
  void updateWidgetPosition();
  void scrollContentsBy(int dx, int dy) override;
  bool event(QEvent* e) override;
  void resizeEvent(QResizeEvent* e) override;
  void showEvent(QShowEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;

private:
  QGraphicsView* View;
  QHeaderView* CreateDeleteHeader;
  QHeaderView* EnabledHeader;
  QStandardItemModel CreateDeleteModel;
  QHeaderView* Header;
  QWidget* CreateDeleteWidget;
  pqAnimationModel* Model;
};

#endif // pqAnimationWidget_h
