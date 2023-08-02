// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimelineItemDelegate_h
#define pqTimelineItemDelegate_h

#include "pqApplicationComponentsModule.h"

#include <QStyledItemDelegate>

#include <memory> // for std::unique_ptr

class pqAnimationScene;

class pqTimelinePainter;

/**
 * pqTimelineItemDelegate draws timeline in cells and add some associated widgets.
 *
 * This handles connection with the active scene to keep the timelines up to date.
 *
 * Start and End scene times are editable and can be locked with dedicated button,
 * integrated in the pqTimelineTrack::TIME item.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelineItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT
  typedef QStyledItemDelegate Superclass;

public:
  pqTimelineItemDelegate(QObject* parent = nullptr, QWidget* parentWidget = nullptr);
  ~pqTimelineItemDelegate() override;

  /**
   * Render the timelines.
   * Depends on the type (time, animation) stored in a custom item role
   */
  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Reimplemented to change row height.
   */
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Reimplemented to handle mouse events.
   * So we can handle clicking on the timeline to edit start / end time.
   */
  bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
    const QModelIndex& index) override;

  /**
   * Reimplemented to handle more mouse events from parent view.
   *
   * editorEvent is useful to get the concerned index and option, but it is pretty
   * restricted. For instance no release event is catched if it occurs outside
   * of the index that received the corresponding press event. Thus we need this
   * eventFilter, to watch the full pqTimelineView.
   */
  bool eventFilter(QObject* watched, QEvent* event) override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the active scene connections.
   */
  void setActiveSceneConnections(pqAnimationScene* scene);

  /**
   * Update cache of start/end time of the scene.
   */
  void updateSceneTimeRange();

Q_SIGNALS:
  void needsRepaint();

protected:
  double Zoom = 1.;

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;

  std::unique_ptr<pqTimelinePainter> TimelinePainter;
};

#endif
