/*=========================================================================

   Program: ParaView
   Module:  pqTimelinePainter.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
 * Drawing also depends on the pqTimelineTrack::TYPE of the current item.
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

  ///@{
  /**
   * Cache information about scene.
   * This is useful to compute tick position, and to do specific label display
   * for locked start and end end.
   */
  // cache scene start time.
  void setSceneStartTime(double time);
  // cache scene end time.
  void setSceneEndTime(double time);
  // cache scene current time.
  void setSceneCurrentTime(double time);
  // cache scene lock start.
  void setSceneLockStart(bool lock);
  // cache scene lock end
  void setSceneLockEnd(bool lock);
  ///@}

  ///@{
  /**
   * Get Start and End labels rectangle.
   */
  // Return true if there is at least 2 rects in cache.
  bool hasStartEndLabels();
  // Return first cached rect.
  QRect getStartLabelRect();
  // Return second cached rect.
  QRect getEndLabelRect();
  ///@}

  /**
   * Return position of the given time.
   * Return -1 if outside the painting area, i.e. if time is not inside [SceneStartTime,
   * SceneEndTime].
   */
  double positionFromTime(double time, const QStyleOptionViewItem& option);

protected:
  ///@{
  /**
   * Paint the different elements of the track.
   * Those are mainly called from internal code.
   * @sa paint
   */
  // paint background
  void paintBackground(QPainter* painter, const QStyleOptionViewItem& option, bool alternate);
  // Paint the whole timeline for given track, i.e. loop over times to draw ticks and optionnal
  // labels. Special code path to ensure Start and End visibility and position.
  void paintTimeline(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    bool paintLabels, const QStyleOptionViewItem& labelsOption);
  // Paint the main timeline, with time labels and a mark for current scene time.
  void paintTimeTrack(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  // Paint a temporal source times. Has a mark for scene time and one for source time.
  void paintSourceTrack(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  // Paint animation track. One mark per keyframe.
  void paintAnimationTrack(
    QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  // Paint time mark for source time
  void paintSourcePipelineTime(
    QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item);
  // Paint time mark for scene time
  void paintSceneCurrentTime(QPainter* painter, const QStyleOptionViewItem& option);
  // Paint a tick, i.e. a mark corresponding to given time. Optionnally paint the associated label.
  // When painting labels, the non-labeled ticks are half-sized, for readability.
  void paintTick(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    double time, bool paintLabels, const QStyleOptionViewItem& labelsOption, const QString& label);
  // Paint a time mark, i.e a vertical line at given position.
  void paintTimeMark(QPainter* painter, const QStyleOptionViewItem& option, double pos);
  // Paint labels as annotation. Return true if the annotation is effectively painted.
  // Do not add label that collides on previously added labels.
  bool paintLabel(QPainter* painter, const QStyleOptionViewItem& option, QStandardItem* item,
    double time, const QString& label);
  ///@}

  ///@{
  /**
   * Extract information from the item data.
   */
  // return true if item is the main time track
  bool isTimeTrack(QStandardItem* item);
  // get item times using relevant data role.
  std::vector<double> getTimes(QStandardItem* item);
  // get source time using relevant data role.
  double getSourceTime(QStandardItem* item);
  // return label for given index
  QString getLabel(QStandardItem* item, int index);
  ///@}

  double SceneCurrentTime = 0;
  double SceneStartTime = 0;
  double SceneEndTime = 1;

  bool SceneLockStart = false;
  bool SceneLockEnd = false;

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
