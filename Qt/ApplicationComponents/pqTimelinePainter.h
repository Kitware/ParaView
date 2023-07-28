// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimelinePainter_h
#define pqTimelinePainter_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

#include <memory> // for std::unique_ptr

class QPainter;
class QStyleOptionViewItem;
class QStandardItem;

/**
 * pqTimelineItemDelegate draws timeline in cells.
 *
 * A timeline is a list of ticks optionally labeled.
 *
 * Inner data of QStandardItemModel are used from the following custom roles:
 *  * pqTimelineItemRole::TIMES : a list of double, to paint a timeline from.
 *  * pqTimelineItemRole::SOURCE_TIME : one double to paint a special mark on given time.
 *  * pqTimelineItemRole::LABELS : (opt) list of labels that can be used when painting a tick
 *
 * Drawing also depends on the pqTimelineItemRole::TYPE of the current item.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelinePainter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTimelinePainter(QObject* parent = nullptr);
  ~pqTimelinePainter() override;

  /**
   * Paint the whole timeline for given index.
   */
  void paint(QPainter* painter, const QModelIndex& index, const QStyleOptionViewItem& option);

  /** @name Scene informations
   * Cache information about scene for display purpose.
   * This is useful to compute tick position, and to do specific label display
   * for locked start and end end.
   */
  ///@{
  /// cache scene start time.
  void setSceneStartTime(double time);
  /// cache scene end time.
  void setSceneEndTime(double time);
  /// cache scene current time.
  void setSceneCurrentTime(double time);
  /// get scene cursor time
  double getSceneCurrentTime();
  /// cache scene lock start.
  void setSceneLockStart(bool lock);
  /// cache scene lock end
  void setSceneLockEnd(bool lock);
  ///@}

  ///@{
  /**
   * Set / Get the time range to display.
   */
  void setDisplayTimeRange(double start, double end);
  QPair<double, double> displayTimeRange();
  ///@}

  /** @name Start and End rectangles
   * Get Start and End labels rectangle.
   */
  ///@{
  /// Return true if there is at least 2 rects in cache.
  bool hasStartEndLabels();
  /// Return first cached rect.
  QRect getStartLabelRect();
  /// Return second cached rect.
  QRect getEndLabelRect();
  ///@}

  ///@{
  /**
   * Return position of the given time.
   * Return -1 if outside the painting area, i.e. if time is not inside [DisplayStartTime,
   * DisplayEndTime].
   */
  double positionFromTime(double time, const QStyleOptionViewItem& option);
  double positionFromTime(double time);
  ///@}

  /**
   * Return the time of the corresponding position.
   * If given index has stored times, return the nearest one.
   */
  ///@{
  double indexTimeFromPosition(
    double pos, const QStyleOptionViewItem& option, const QModelIndex& index);
  double timeFromPosition(double pos, const QStyleOptionViewItem& option);
  double timeFromPosition(double pos);
  ///@}

  /** @name Items infos
   * Extract information from the item data.
   */
  ///@{
  /// return true if item is the main time track
  bool isTimeTrack(QStandardItem* item);
  /// return true if item is an animation track
  bool isAnimationTrack(QStandardItem* item);
  /// get item times using relevant data role.
  std::vector<double> getTimes(QStandardItem* item);
  /// get source time using relevant data role.
  double getSourceTime(QStandardItem* item);
  /// return label for given index
  QString getLabel(QStandardItem* item, int index);
  ///@}

protected:
  /** @name Paint methods
   * Paint the different elements of the track.
   * Those are mainly called from internal code.
   * @sa paint
   */
  ///@{
  /// paint background
  void paintBackground(QPainter* painter, const QStyleOptionViewItem& option, bool alternate);
  /// Paint the whole timeline for given track, i.e. loop over times to draw ticks and optionnal
  /// labels. Special code path to ensure Start and End visibility and position.
  void paintTimeline(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    bool paintLabels, const QStyleOptionViewItem& labelsOption);
  /// Paint the main timeline, with time labels and a mark for current scene time.
  void paintTimeTrack(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  /// a temporal source times. Has a mark for scene time and one for source time.
  void paintSourceTrack(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  /// animation track. One mark per keyframe.
  void paintAnimationTrack(
    QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  /// time mark for source time
  void paintSourcePipelineTime(
    QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  /// Paint time mark for scene time
  void paintSceneCurrentTime(QPainter* painter, const QStyleOptionViewItem& option);
  /// a tick, i.e. a mark corresponding to given time. Optionnally paint the associated label.
  /// When painting labels, the non-labeled ticks are half-sized, for readability.
  /// Return true if the label was painted.
  bool paintTick(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    double time, bool paintLabels, const QStyleOptionViewItem& labelsOption, const QString& label);
  /// a time mark, i.e a vertical line at given position.
  void paintTimeMark(QPainter* painter, const QStyleOptionViewItem& option, double pos);
  /// Paint labels as annotation. Return true if the annotation is effectively painted.
  /// Do not add label that collides on previously added labels.
  bool paintLabel(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    double time, const QString& label);
  ///@}

  double SceneCurrentTime = 0;
  double SceneStartTime = 0;
  double SceneEndTime = 1;

  double DisplayStartTime = 0.;
  double DisplayEndTime = 1.;

  bool SceneLockStart = false;
  bool SceneLockEnd = false;

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
