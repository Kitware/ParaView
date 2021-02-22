/*=========================================================================

   Program: ParaView
   Module:    pqAnimationModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqAnimationModel.h"

#include "assert.h"

#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QStyle>

#include "pqAnimationKeyFrame.h"
#include "pqAnimationTrack.h"
#include "pqCheckBoxPixMaps.h"

#include <cassert>
#include <iostream>

constexpr int MouseWheelStepsFactor = 120;

//-----------------------------------------------------------------------------
pqAnimationModel::pqAnimationModel(QGraphicsView* p)
  : QGraphicsScene(QRectF(0, 0, 400, 16 * 6), p)
{
  QObject::connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(resizeTracks()));
  p->installEventFilter(this);
  this->Header.appendRow(new QStandardItem());
  this->Header.setHeaderData(0, Qt::Vertical, "Time", Qt::DisplayRole);

  this->EnabledHeader.appendRow(new QStandardItem());
  this->EnabledHeader.setHeaderData(0, Qt::Vertical, "  ", Qt::DisplayRole);

  this->CheckBoxPixMaps = new pqCheckBoxPixMaps(p);
}

//-----------------------------------------------------------------------------
pqAnimationModel::~pqAnimationModel()
{
  while (this->Tracks.size())
  {
    this->removeTrack(this->Tracks[0]);
  }
  delete this->CheckBoxPixMaps;
  this->CheckBoxPixMaps = nullptr;
}

//-----------------------------------------------------------------------------
QAbstractItemModel* pqAnimationModel::header()
{
  return &this->Header;
}

//-----------------------------------------------------------------------------
QAbstractItemModel* pqAnimationModel::enabledHeader()
{
  return &this->EnabledHeader;
}

//-----------------------------------------------------------------------------
int pqAnimationModel::count()
{
  return this->Tracks.size();
}

//-----------------------------------------------------------------------------
pqAnimationTrack* pqAnimationModel::track(int i)
{
  if (i >= 0 && i < this->Tracks.size())
  {
    return this->Tracks[i];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqAnimationTrack* pqAnimationModel::addTrack(pqAnimationTrack* trackToAdd)
{
  pqAnimationTrack* t = trackToAdd ? trackToAdd : new pqAnimationTrack(this);
  this->Tracks.append(t);
  this->addItem(t);
  this->resizeTracks();

  this->Header.appendRow(new QStandardItem());
  this->EnabledHeader.appendRow(new QStandardItem());
  QObject::connect(t, SIGNAL(propertyChanged()), this, SLOT(trackNameChanged()));

  QObject::connect(t, SIGNAL(enabledChanged()), this, SLOT(enabledChanged()));
  return t;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::removeTrack(pqAnimationTrack* t)
{
  int idx = this->Tracks.indexOf(t);
  if (idx != -1)
  {
    this->Tracks.removeAt(idx);
    this->removeItem(t);
    this->Header.removeRow(idx + 1); // off by one for time header item
    this->EnabledHeader.removeRow(idx + 1);
    delete t;
    this->resizeTracks();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::resizeTracks()
{
  // give each track some height more than text height
  // if total tracks exceeds sceneRect, increase the sceneRect

  int i;
  int num = this->Tracks.size();
  QRectF rect = this->sceneRect();
  double rh = this->rowHeight();
  double requiredHeight = rh * (num + 1);
  if (rect.height() != requiredHeight)
  {
    this->setSceneRect(rect.left(), rect.top(), rect.width(), requiredHeight);
    return;
  }

  rh = (requiredHeight - 1) / double(num + 1);
  double h = rh;

  double trackLeft = this->positionFromTime(this->ZoomStartTime);
  double trackRight = this->positionFromTime(this->ZoomEndTime);

  for (i = 0; i < num; i++)
  {
    this->Tracks[i]->setBoundingRect(QRectF(trackLeft, h, trackRight, rh));
    this->zoomTrack(this->Tracks[i]);
    h += rh;
  }
}

//-----------------------------------------------------------------------------
pqAnimationModel::ModeType pqAnimationModel::mode() const
{
  return this->Mode;
}

//-----------------------------------------------------------------------------
int pqAnimationModel::ticks() const
{
  return this->Ticks;
}

//-----------------------------------------------------------------------------
int pqAnimationModel::currentTicks() const
{
  return this->Mode == Custom ? this->CustomTicks.size() : this->ticks();
}

//-----------------------------------------------------------------------------
double pqAnimationModel::currentTime() const
{
  return this->CurrentTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::startTime() const
{
  return this->StartTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::endTime() const
{
  return this->EndTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::zoomStartTime() const
{
  return this->ZoomStartTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::zoomEndTime() const
{
  return this->ZoomEndTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::zoomFactor() const
{
  return this->ZoomFactor;
}

//-----------------------------------------------------------------------------
bool pqAnimationModel::interactive() const
{
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setEnabledHeaderToolTip(const QString& val)
{
  if (this->EnabledHeaderToolTip != val)
  {
    this->EnabledHeaderToolTip = val;
    this->enabledChanged();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setRowHeight(int rh)
{
  this->RowHeight = rh;
  this->resizeTracks();
}

//-----------------------------------------------------------------------------
int pqAnimationModel::rowHeight() const
{
  return this->RowHeight;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setMode(pqAnimationModel::ModeType m)
{
  this->Mode = m;
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setTicks(int f)
{
  this->Ticks = f;
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setTickMarks(int cnt, double* times)
{
  this->CustomTicks.clear();
  for (int cc = 0; cc < cnt; cc++)
  {
    this->CustomTicks.push_back(times[cc]);
  }
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setCurrentTime(double t)
{
  this->CurrentTime = t;
  this->NewCurrentTime = t;
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setStartTime(double t)
{
  this->StartTime = t;
  this->ZoomStartTime = this->StartTime;
  this->ZoomFactor = 1;
  this->resizeTracks();
  this->update();
  emit this->zoomChanged();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setEndTime(double t)
{
  this->EndTime = t;
  this->ZoomEndTime = this->EndTime;
  this->ZoomFactor = 1;
  this->resizeTracks();
  this->update();
  emit this->zoomChanged();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setInteractive(bool b)
{
  this->Interactive = b;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::positionFromTime(double time)
{
  QRectF sr = this->sceneRect();
  double fraction = (time - this->ZoomStartTime) / (this->ZoomEndTime - this->ZoomStartTime);
  return fraction * (sr.width() - 1) + sr.left();
}

//-----------------------------------------------------------------------------
double pqAnimationModel::timeFromPosition(double pos)
{
  QRectF sr = this->sceneRect();
  double fraction = (pos - sr.left()) / (sr.width() - 1);
  return fraction * (this->ZoomEndTime - this->ZoomStartTime) + this->ZoomStartTime;
}

//-----------------------------------------------------------------------------
double pqAnimationModel::timeFromTick(int tick)
{
  if (this->Mode == Custom)
  {
    assert(tick <= this->CustomTicks.size());
    return this->CustomTicks[tick];
  }

  double fraction = tick / (this->currentTicks() - 1.0);
  return fraction * (this->EndTime - this->StartTime) + this->StartTime;
}

//-----------------------------------------------------------------------------
int pqAnimationModel::tickFromTime(double time)
{
  if (this->Mode == Custom)
  {
    double error = 1.0e+299;
    int index = -1;
    int cc = 0;
    foreach (double tick_time, this->CustomTicks)
    {
      if (error > qAbs(tick_time - time))
      {
        error = qAbs(tick_time - time);
        index = cc;
      }
      cc++;
    }
    if (index != -1)
    {
      return index;
    }
  }

  double fraction = (time - this->StartTime) / (this->EndTime - this->StartTime);
  return qRound(fraction * (this->Ticks - 1.0));
}

//-----------------------------------------------------------------------------
QPolygonF pqAnimationModel::timeBarPoly(double time)
{
  int rh = this->rowHeight();
  QRectF sr = this->sceneRect();

  double pos = this->positionFromTime(time);
  QVector<QPointF> polyPoints;
  polyPoints.append(QPointF(pos - 4, rh - 7));
  polyPoints.append(QPointF(pos - 4, rh - 4));
  polyPoints.append(QPointF(pos - 1, rh - 1));
  polyPoints.append(QPointF(pos - 1, sr.height() + sr.top() - 2));
  polyPoints.append(QPointF(pos + 1, sr.height() + sr.top() - 2));
  polyPoints.append(QPointF(pos + 1, rh - 1));
  polyPoints.append(QPointF(pos + 4, rh - 4));
  polyPoints.append(QPointF(pos + 4, rh - 7));
  return QPolygonF(polyPoints);
}

//-----------------------------------------------------------------------------
void pqAnimationModel::drawForeground(QPainter* painter, const QRectF&)
{
  painter->save();

  QRectF sr = this->sceneRect();
  int rh = this->rowHeight();

  QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
  const QRectF labelRect = QRectF(sr.left(), sr.top(), sr.width() - 1, rh);

  // make background for time labels white
  painter->save();
  painter->setBrush(this->palette().base());
  painter->setPen(Qt::NoPen);
  painter->drawRect(labelRect);
  painter->restore();

  // show rough time labels for all time modes
  // TODO  would be nice to improve the time labels
  //       match them with ticks in sequence mode
  //       don't draw labels with 'e' formatting, if its simpler or more compact
  QFontMetrics metrics(view->font());
  int num = qRound(labelRect.width() / (9 * metrics.maxWidth()));
  num = num == 0 ? 1 : num;

  painter->save();
  painter->setPen(this->palette().color(QPalette::Text));

  QList<const QRectF*> priorities;

  // draw the new current time label if needed
  QRectF newCurrentTimeLabRect;
  if (this->NewCurrentTime != this->CurrentTime && this->CurrentKeyFrameGrabbed)
  {
    newCurrentTimeLabRect =
      this->drawTimeLabel(this->NewCurrentTime, labelRect, painter, metrics, priorities);
  }

  // draw the current time label
  priorities.append(&newCurrentTimeLabRect);
  QRectF currentTimeLabRect =
    this->drawTimeLabel(this->CurrentTime, labelRect, painter, metrics, priorities);

  // draw the other labels
  priorities.append(&currentTimeLabRect);

  this->drawTimeLabel(this->ZoomStartTime, labelRect, painter, metrics, priorities);

  for (int i = 1; i < num; i++)
  {
    double time = this->ZoomStartTime + ((this->ZoomEndTime - this->ZoomStartTime) * i) / num;

    this->drawTimeLabel(time, labelRect, painter, metrics, priorities);
  }

  this->drawTimeLabel(this->ZoomEndTime, labelRect, painter, metrics, priorities);

  painter->restore();

  // if sequence, draw a tick mark for each frame
  if ((this->mode() == Sequence || this->mode() == Custom) && this->currentTicks() > 2)
  {
    for (int i = 0, max = this->currentTicks(); i < max; i++)
    {
      double tickTime = this->timeFromTick(i);
      if (tickTime > ZoomStartTime)
      {
        if (tickTime > ZoomEndTime)
        {
          break;
        }
        double tickPos = this->positionFromTime(tickTime);
        QLineF line(tickPos, labelRect.height(), tickPos, labelRect.height() - 3.0);
        painter->drawLine(line);
      }
    }
  }

  // draw current time bar
  QPen pen = painter->pen();
  pen.setJoinStyle(Qt::MiterJoin);
  painter->setPen(pen);
  painter->setBrush(this->palette().text());

  QPolygonF poly = this->timeBarPoly(this->CurrentTime);
  painter->drawPolygon(poly);

  if (this->CurrentKeyFrameGrabbed)
  {
    double pos = this->positionFromTime(this->NewCurrentTime);
    QVector<QPointF> pts;
    pts.append(QPointF(pos - 1, rh - 1));
    pts.append(QPointF(pos - 1, sr.height() + sr.top() - 2));
    pts.append(QPointF(pos + 1, sr.height() + sr.top() - 2));
    pts.append(QPointF(pos + 1, rh - 1));
    painter->setBrush(QColor(200, 200, 200));
    painter->drawPolygon(QPolygonF(pts));
  }

  painter->restore();
}

//-----------------------------------------------------------------------------
QRectF pqAnimationModel::drawTimeLabel(double time, const QRectF& row, QPainter* painter,
  const QFontMetrics& metrics, QList<const QRectF*> const& priorities)
{
  QString timeLabString = QString::number(time, this->TimeNotation.toLatin1(), this->TimePrecision);
  double timeLabSize = metrics.horizontalAdvance(timeLabString);
  double currentTimePos = this->positionFromTime(time);
  double halfLabWidth = timeLabSize / 2.0;
  double timeLabPos;

  // if the label is outside of the current zoom frame
  if (currentTimePos - halfLabWidth < row.left() - halfLabWidth ||
    currentTimePos + halfLabWidth > row.right() + halfLabWidth)
  {
    return QRectF();
  }

  if (currentTimePos - halfLabWidth < row.left())
  {
    timeLabPos = row.left();
  }
  else if (currentTimePos + halfLabWidth > row.right())
  {
    timeLabPos = row.right() - timeLabSize;
  }
  else
  {
    timeLabPos = currentTimePos - halfLabWidth;
  }

  QRectF timeLabRect(timeLabPos, row.top(), timeLabSize, row.height());

  // check the priorities to avoid drawing labels on top of others
  const QRectF* priority;
  foreach (priority, priorities)
  {
    if (timeLabRect.intersects(*priority))
    {
      return timeLabRect;
    }
  }

  painter->drawText(timeLabRect, Qt::AlignCenter, timeLabString);

  return timeLabRect;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::updateNewTime(QGraphicsSceneMouseEvent* mouseEvent)
{
  QPointF pos = mouseEvent->scenePos();

  double time = this->timeFromPosition(pos.x());

  // snap to ticks in sequence mode
  if (this->mode() == Sequence || this->mode() == Custom)
  {
    int tick = this->tickFromTime(time);
    time = this->timeFromTick(tick);
  }
  else
  {
    // snap to nearby snap hints (if any)
    for (int i = 0; i < this->SnapHints.size(); i++)
    {
      if (qAbs(this->positionFromTime(this->SnapHints[i]) - this->positionFromTime(time)) < 3)
      {
        time = this->SnapHints[i];
        break;
      }
    }
  }

  // clamp to start/end time
  time = qMax(time, this->InteractiveRange.first);
  time = qMin(time, this->InteractiveRange.second);

  this->NewCurrentTime = time;

  if (this->NewCurrentTime != this->CurrentTime && this->CurrentTimeGrabbed)
  {
    // update the current time
    emit this->currentTimeSet(this->NewCurrentTime);
    return;
  }
  this->update();
}

//-----------------------------------------------------------------------------
bool pqAnimationModel::eventFilter(QObject* w, QEvent* e)
{
  if (e->type() == QEvent::Resize)
  {
    QGraphicsView* v = qobject_cast<QGraphicsView*>(w);
    QRect sz = v->contentsRect();
    this->setSceneRect(0, 0, sz.width(), (this->Tracks.size() + 1) * this->rowHeight());
    v->ensureVisible(this->sceneRect(), 0, 0);
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::trackNameChanged()
{
  QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
  QFontMetrics metrics(view->font());

  for (int i = 0; i < this->Tracks.size(); i++)
  {
    this->Header.setHeaderData(i + 1, Qt::Vertical, this->Tracks[i]->property(), Qt::DisplayRole);
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::enabledChanged()
{
  for (int i = 0; i < this->Tracks.size(); i++)
  {
    this->EnabledHeader.setHeaderData(i + 1, Qt::Vertical,
      this->Tracks[i]->isEnabled() ? this->CheckBoxPixMaps->getPixmap(Qt::Checked, false)
                                   : this->CheckBoxPixMaps->getPixmap(Qt::Unchecked, false),
      Qt::DecorationRole);
    this->EnabledHeader.setHeaderData(
      i + 1, Qt::Vertical, this->EnabledHeaderToolTip, Qt::ToolTipRole);
  }
}

//-----------------------------------------------------------------------------
bool pqAnimationModel::hitTestCurrentTimePoly(const QPointF& pos)
{
  QPolygonF poly = this->timeBarPoly(this->CurrentTime);
  QRectF rect = poly.boundingRect().adjusted(-4, -1, 4, 1);
  return rect.contains(pos);
}

//-----------------------------------------------------------------------------
pqAnimationTrack* pqAnimationModel::hitTestTracks(const QPointF& pos)
{
  QList<QGraphicsItem*> hitItems = this->items(pos);
  foreach (QGraphicsItem* i, hitItems)
  {
    if (this->Tracks.contains(static_cast<pqAnimationTrack*>(i)))
    {
      return static_cast<pqAnimationTrack*>(i);
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqAnimationKeyFrame* pqAnimationModel::hitTestKeyFrame(pqAnimationTrack* t, const QPointF& pos)
{
  if (t)
  {
    for (int i = 0; i < t->count(); i++)
    {
      pqAnimationKeyFrame* kf = t->keyFrame(i);
      double keyPos1 =
        this->positionFromTime(this->normalizedTimeToTime(kf->normalizedStartTime()));
      double keyPos2 = this->positionFromTime(this->normalizedTimeToTime(kf->normalizedEndTime()));
      if (pos.x() >= keyPos1 && pos.x() <= keyPos2)
      {
        return kf;
      }
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void pqAnimationModel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
  if (mouseEvent->button() == Qt::LeftButton)
  {
    QPointF pos = mouseEvent->scenePos();
    pqAnimationTrack* t = hitTestTracks(pos);
    if (t)
    {
      emit trackSelected(t);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
  if (!this->Interactive ||
    (mouseEvent->button() != Qt::LeftButton && mouseEvent->button() != Qt::MiddleButton) ||
    this->TimeLineGrabbed)
  {
    return;
  }

  if (mouseEvent->button() == Qt::MiddleButton ||
    mouseEvent->modifiers().testFlag(Qt::ControlModifier))
  {
    if (!this->CurrentKeyFrameGrabbed && !this->CurrentTimeGrabbed)
    {
      QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
      view->setCursor(QCursor(Qt::ClosedHandCursor));
      this->TimeLineGrabbedPosition = mouseEvent->scenePos().x();
      this->OldZoomStartTime = this->ZoomStartTime;
      this->TimeLineGrabbed = true;
      return;
    }
  }

  // see if current time is grabbed or if the time header is clicked
  QPointF pos = mouseEvent->scenePos();

  // get the header rectangle
  QRectF sr = this->sceneRect();
  int rh = this->rowHeight();
  const QRectF labelRect = QRectF(sr.left(), sr.top(), sr.width() - 1, rh);

  if (this->hitTestCurrentTimePoly(pos) || labelRect.contains(pos))
  {
    this->CurrentTimeGrabbed = true;
    this->InteractiveRange.first = this->ZoomStartTime;
    this->InteractiveRange.second = this->ZoomEndTime;
  }

  // update the new current time
  this->updateNewTime(mouseEvent);

  // see if any keyframe is grabbed
  if (!this->CurrentTimeGrabbed)
  {
    pqAnimationTrack* t = hitTestTracks(pos);
    pqAnimationKeyFrame* kf = hitTestKeyFrame(t, pos);

    if (t && kf)
    {
      int whichkf = 0;
      for (whichkf = 0; whichkf < t->count(); whichkf++)
      {
        if (t->keyFrame(whichkf) == kf)
        {
          break;
        }
      }

      if (kf)
      {
        double keyPos1 =
          this->positionFromTime(this->normalizedTimeToTime(kf->normalizedStartTime()));
        double keyPos2 =
          this->positionFromTime(this->normalizedTimeToTime(kf->normalizedEndTime()));
        if (qAbs(keyPos1 - pos.x()) < 10)
        {
          this->CurrentTrackGrabbed = t;
          this->CurrentKeyFrameGrabbed = kf;
          this->CurrentKeyFrameEdge = 0;
        }
        else if (qAbs(keyPos2 - pos.x()) < 10)
        {
          whichkf++;
          this->CurrentTrackGrabbed = t;
          this->CurrentKeyFrameGrabbed = kf;
          this->CurrentKeyFrameEdge = 1;
          this->InteractiveRange.first = this->ZoomStartTime;
          this->InteractiveRange.second = this->ZoomEndTime;
        }

        if (whichkf > 0)
        {
          this->InteractiveRange.first =
            this->normalizedTimeToTime(t->keyFrame(whichkf - 1)->normalizedStartTime());
        }
        else
        {
          this->InteractiveRange.first = this->ZoomStartTime;
        }

        if (whichkf < t->count())
        {
          this->InteractiveRange.second =
            this->normalizedTimeToTime(t->keyFrame(whichkf)->normalizedEndTime());
        }
        else
        {
          this->InteractiveRange.second = this->ZoomEndTime;
        }
      }
    }
  }

  // gather some snap hints from the current time
  // and all the keyframes
  if (this->CurrentTimeGrabbed || this->CurrentTrackGrabbed)
  {
    this->SnapHints.append(this->CurrentTime);

    for (int i = 0; i < this->count(); i++)
    {
      pqAnimationTrack* t = this->track(i);
      for (int j = 0; j < t->count(); j++)
      {
        pqAnimationKeyFrame* kf = t->keyFrame(j);
        this->SnapHints.append(this->normalizedTimeToTime(kf->normalizedStartTime()));
        this->SnapHints.append(this->normalizedTimeToTime(kf->normalizedEndTime()));
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
  if (!this->Interactive)
  {
    return;
  }

  QPointF pos = mouseEvent->scenePos();

  if (this->CurrentTimeGrabbed || this->CurrentKeyFrameGrabbed)
  {
    this->updateNewTime(mouseEvent);
    return;
  }

  if (mouseEvent->buttons() == Qt::NoButton)
  {
    // we haven't gone in any interaction mode yet,
    // so lets adjust the cursor to give indication of being
    // able to interact if the mouse was pressed at this location
    QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());

    if (this->hitTestCurrentTimePoly(pos))
    {
      view->setCursor(QCursor(Qt::SizeHorCursor));
      return;
    }

    // see if we're at the edge of any keyframe
    pqAnimationTrack* t = hitTestTracks(pos);
    pqAnimationKeyFrame* kf = hitTestKeyFrame(t, pos);
    if (kf)
    {
      double keyPos1 =
        this->positionFromTime(this->normalizedTimeToTime(kf->normalizedStartTime()));
      double keyPos2 = this->positionFromTime(this->normalizedTimeToTime(kf->normalizedEndTime()));
      if (qAbs(keyPos1 - pos.x()) < 10 || qAbs(keyPos2 - pos.x()) < 10)
      {
        view->setCursor(QCursor(Qt::SizeHorCursor));
        return;
      }
    }
    // in case cursor was changed elsewhere
    view->setCursor(QCursor());
  }
  else if (this->TimeLineGrabbed)
  {
    // Move the start and the end of the Zoom
    double timeDiff = this->timeFromPosition(mouseEvent->scenePos().x()) -
      this->timeFromPosition(this->TimeLineGrabbedPosition);

    this->positionZoom(this->OldZoomStartTime - timeDiff);

    emit this->zoomChanged();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
  if (TimeLineGrabbed)
  {
    TimeLineGrabbed = false;

    QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
    // in case cursor was changed elsewhere
    view->setCursor(QCursor());
    return;
  }

  if (mouseEvent->button() == Qt::LeftButton)
  {
    if (this->CurrentTimeGrabbed)
    {
      this->CurrentTimeGrabbed = false;
    }

    if (this->CurrentKeyFrameGrabbed)
    {
      emit this->keyFrameTimeChanged(this->CurrentTrackGrabbed, this->CurrentKeyFrameGrabbed,
        this->CurrentKeyFrameEdge, this->NewCurrentTime);

      this->CurrentTrackGrabbed = nullptr;
      this->CurrentKeyFrameGrabbed = nullptr;
      this->NewCurrentTime = this->CurrentTime;
      this->update();
    }

    this->SnapHints.clear();

    QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
    // in case cursor was changed elsewhere
    view->setCursor(QCursor());
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::positionZoom(double zoomStartTime)
{
  double timeDistance = (this->EndTime - this->StartTime) / this->ZoomFactor;

  this->ZoomStartTime = zoomStartTime;
  this->ZoomEndTime = this->ZoomStartTime + timeDistance;

  if (this->ZoomStartTime < this->StartTime)
  {
    this->ZoomStartTime = this->StartTime;
    this->ZoomEndTime = this->ZoomStartTime + timeDistance;
  }
  else if (this->ZoomEndTime > this->EndTime)
  {
    this->ZoomEndTime = this->EndTime;
    this->ZoomStartTime = this->ZoomEndTime - timeDistance;
  }

  for (int i = 0; i < this->count(); i++)
  {
    pqAnimationTrack* t = this->track(i);
    this->zoomTrack(t);
  }

  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::wheelEvent(QGraphicsSceneWheelEvent* wheelEvent)
{
  if (!this->Interactive || !wheelEvent->modifiers().testFlag(Qt::ControlModifier) ||
    wheelEvent->buttons() != Qt::NoButton)
  {
    return;
  }

  double steps = wheelEvent->delta() / MouseWheelStepsFactor;
  double newZoomStart;

  this->ZoomFactor += steps / 2;

  if (this->ZoomFactor <= 1)
  {
    this->ZoomFactor = 1;
    newZoomStart = this->StartTime;
  }
  else
  {
    double timeTarget = timeFromPosition(wheelEvent->scenePos().x());

    double fraction =
      (timeTarget - this->ZoomStartTime) / (this->ZoomEndTime - this->ZoomStartTime);

    double timeDistance = (this->EndTime - this->StartTime) / this->ZoomFactor;

    newZoomStart = timeTarget - (fraction * timeDistance);
  }

  this->positionZoom(newZoomStart);

  emit this->zoomChanged();
}

//-----------------------------------------------------------------------------
double pqAnimationModel::timeToNormalizedTime(double t) const
{
  return (t - this->startTime()) / (this->endTime() - this->startTime());
}

//-----------------------------------------------------------------------------
double pqAnimationModel::normalizedTimeToTime(double t) const
{
  return t * (this->endTime() - this->startTime()) + this->startTime();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setTimePrecision(int precision)
{
  this->TimePrecision = precision;
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationModel::setTimeNotation(const QChar& notation)
{
  QString possibilities = QString("eEfgG");
  if (possibilities.contains(notation) && this->TimeNotation != notation)
  {
    this->TimeNotation = notation;
    this->update();
  }
}

//-----------------------------------------------------------------------------
void pqAnimationModel::zoomTrack(pqAnimationTrack* track)
{
  for (int j = 0; j < track->count(); j++)
  {
    pqAnimationKeyFrame* kf = track->keyFrame(j);
    double startPos = this->positionFromTime(this->normalizedTimeToTime(kf->normalizedStartTime()));
    double endPos = this->positionFromTime(this->normalizedTimeToTime(kf->normalizedEndTime()));
    kf->adjustRect(startPos, endPos);
  }
}
