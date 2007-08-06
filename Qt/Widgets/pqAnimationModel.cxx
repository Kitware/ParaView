/*=========================================================================

   Program: ParaView
   Module:    pqAnimationModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include <QPainter>
#include <QGraphicsView>
#include <QEvent>
#include <QStyle>
#include <QGraphicsSceneMouseEvent>

#include "pqAnimationTrack.h"

pqAnimationModel::pqAnimationModel(QGraphicsView* p)
  : QGraphicsScene(QRectF(0,0,400,16*6), p),
  Mode(Real), Ticks(10), CurrentTime(0), StartTime(0), EndTime(1)
{
  QObject::connect(this, SIGNAL(sceneRectChanged(QRectF)),
                   this, SLOT(resizeTracks()));
  p->installEventFilter(this);
  this->Header.appendRow(new QStandardItem());
  this->Header.setHeaderData(0, Qt::Vertical, "Time", Qt::DisplayRole);

}

pqAnimationModel::~pqAnimationModel()
{
}

QAbstractItemModel* pqAnimationModel::header()
{
  return &this->Header;
}

int pqAnimationModel::count()
{
  return this->Tracks.size();
}
pqAnimationTrack* pqAnimationModel::track(int i)
{
  return this->Tracks[i];
}

pqAnimationTrack* pqAnimationModel::addTrack()
{
  pqAnimationTrack* t = new pqAnimationTrack(this);
  this->Tracks.append(t);
  this->addItem(t);
  this->resizeTracks();

  this->Header.appendRow(new QStandardItem());
  QObject::connect(t, SIGNAL(propertyChanged()),
                   this, SLOT(trackNameChanged()));
  
  return t;
}

void pqAnimationModel::removeTrack(pqAnimationTrack* t)
{
  int idx = this->Tracks.indexOf(t);
  if(idx != -1)
    {
    this->Tracks.removeAt(idx);
    this->removeItem(t);
    this->Header.removeRow(idx+1);  // off by one for time header item
    delete t;
    this->resizeTracks();
    }
}

void pqAnimationModel::resizeTracks()
{
  // give each track some height more than text height
  // if total tracks exceeds sceneRect, increase the sceneRect

  int i;
  int num = this->Tracks.size();
  QRectF rect = this->sceneRect();
  double rh = this->rowHeight();
  double requiredHeight = rh * (num+1);
  if(rect.height() < requiredHeight)
    {
    this->setSceneRect(rect.left(), rect.top(), rect.width(), requiredHeight);
    return;
    }
  
  rh = (requiredHeight -1)/double(num+1);
  double h = rh;
  for(i=0; i<num; i++)
    {
    this->Tracks[i]->setBoundingRect(QRectF(rect.left(), h, rect.width() - 1, rh));
    h += rh;
    }
}

pqAnimationModel::ModeType pqAnimationModel::mode() const
{
  return this->Mode;
}
int pqAnimationModel::ticks() const
{
  return this->Ticks;
}
double pqAnimationModel::currentTime() const
{
  return this->CurrentTime;
}
double pqAnimationModel::startTime() const
{
  return this->StartTime;
}
double pqAnimationModel::endTime() const
{
  return this->EndTime;
}

void pqAnimationModel::setRowHeight(int rh)
{
  this->RowHeight = rh;
  this->resizeTracks();
}

int pqAnimationModel::rowHeight() const
{
  return this->RowHeight;
}

void pqAnimationModel::setMode(pqAnimationModel::ModeType m)
{
  this->Mode = m;
  this->update();
}
void pqAnimationModel::setTicks(int f)
{
  this->Ticks = f;
  this->update();
}
void pqAnimationModel::setCurrentTime(double t)
{
  this->CurrentTime = t;
  this->update();
}
void pqAnimationModel::setStartTime(double t)
{
  this->StartTime = t;
  this->resizeTracks();
  this->update();
}
void pqAnimationModel::setEndTime(double t)
{
  this->EndTime = t;
  this->resizeTracks();
  this->update();
}

void pqAnimationModel::drawForeground(QPainter* painter, const QRectF& )
{
  painter->save();
  
  QRectF sr = this->sceneRect();
  int rh = this->rowHeight();


  QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
  const QRectF labelRect = QRectF(sr.left(), sr.top(), sr.width()-1, rh);

  // make background for time labels white
  painter->save();
  painter->setBrush(QColor(255,255,255));
  painter->setPen(QColor());
  painter->drawRect(labelRect);
  painter->restore();
  
  // show rough time labels for all time modes
  // TODO  would be nice to improve the time labels
  //       match them with ticks in sequence mode
  //       don't draw labels in 'e' mode, if its simpler or more compact
  QFontMetrics metrics(view->font());
  int num = qRound(labelRect.width() / (9 * metrics.maxWidth()));
  num = num == 0 ? 1 : num;
  double w = labelRect.width() / num;

  painter->drawText(QRectF(labelRect.left(), labelRect.top(), w/2.0, rh), 
                    Qt::AlignLeft | Qt::AlignVCenter,
                    QString("%1").arg(this->StartTime, 5, 'e', 3));

  for(int i=1; i<num; i++)
    {
    double time = this->StartTime + (this->EndTime - this->StartTime) * (double)i/(double)num;
    double left = labelRect.left() + w / 2.0 + w * (i-1);
    painter->drawText(QRectF(left, labelRect.top(), w, rh), 
                      Qt::AlignCenter, QString("%1").arg(time, 5, 'e', 3));
    }
  
  painter->drawText(QRectF(labelRect.right() - w/2.0, labelRect.top(), w/2.0, rh), 
                    Qt::AlignRight | Qt::AlignVCenter,
                    QString("%1").arg(this->EndTime, 5, 'e', 3));

  
  // if sequence, draw a tick mark for each frame
  if(this->mode() == Sequence && this->Ticks > 1)
    {
    double spacing = labelRect.width() / ((double)this->Ticks-1);
    QLineF line(labelRect.left(), labelRect.height(),
                labelRect.left(), labelRect.height()-3.0);
    for(int i=0; i<this->Ticks; i++)
      {
      painter->drawLine(line);
      line.translate(spacing, 0);
      }
    }
  
  // draw current time bar
  QPen pen = painter->pen();
  pen.setJoinStyle(Qt::MiterJoin);
  painter->setPen(pen);
  painter->setBrush(QColor(0,0,0));

  double fraction = this->CurrentTime / (this->EndTime - this->StartTime);
  double pos = fraction * (sr.width()-1) + sr.left();
  QVector<QPointF> polyPoints;
  polyPoints.append(QPointF(pos, rh-1));
  polyPoints.append(QPointF(pos + 4, rh - 4));
  polyPoints.append(QPointF(pos + 4, rh - 7));
  polyPoints.append(QPointF(pos - 4, rh - 7));
  polyPoints.append(QPointF(pos - 4, rh - 4));
  QPolygonF poly(polyPoints);
  painter->drawPolygon(poly);
  
  pen.setWidth(3);
  painter->setPen(pen);

  QPointF pt1(pos, rh-1);
  QPointF pt2(pos, sr.height() + sr.top()-2);
  painter->drawLine(pt1, pt2);


  painter->restore();
}
  
bool pqAnimationModel::eventFilter(QObject* w, QEvent* e)
{
  if(e->type() == QEvent::Resize)
    {
    QGraphicsView* v = qobject_cast<QGraphicsView*>(w);
    QRect sz = v->contentsRect();
    this->setSceneRect(0, 0, sz.width(), (this->Tracks.size()+1) * this->rowHeight());
    v->ensureVisible(this->sceneRect(), 0, 0);
    }
  if(e->type() == QEvent::FontChange)
    {
    this->resizeTracks();
    }
  return false;
}

void pqAnimationModel::trackNameChanged()
{
  QGraphicsView* view = qobject_cast<QGraphicsView*>(this->parent());
  QFontMetrics metrics(view->font());

  for(int i=0; i<this->Tracks.size(); i++)
    {
    this->Header.setHeaderData(i+1, Qt::Vertical, this->Tracks[i]->property(),
                               Qt::DisplayRole);
    }
}

void pqAnimationModel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
  if(mouseEvent->button() == Qt::LeftButton)
    {
    QPointF pos = mouseEvent->scenePos();
    QList<QGraphicsItem*> hitItems = this->items(pos);
    foreach(QGraphicsItem* i, hitItems)
      {
      if(this->Tracks.contains(static_cast<pqAnimationTrack*>(i)))
        {
        emit trackSelected(static_cast<pqAnimationTrack*>(i));
        return;
        }
      }
    }
}

