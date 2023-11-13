// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_12_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "pqAnimationTrack.h"

#include <QPainter>
#include <QPalette>
#include <QWidget>

#include "pqAnimationKeyFrame.h"

//-----------------------------------------------------------------------------
pqAnimationTrack::pqAnimationTrack(QObject* p)
  : QObject(p)
  , Rect(0, 0, 1, 1)
{
}

//-----------------------------------------------------------------------------
pqAnimationTrack::~pqAnimationTrack()
{
  while (this->Frames.count())
  {
    this->removeKeyFrame(this->Frames[0]);
  }
}

//-----------------------------------------------------------------------------
int pqAnimationTrack::count()
{
  return this->Frames.size();
}

pqAnimationKeyFrame* pqAnimationTrack::keyFrame(int i)
{
  return this->Frames[i];
}

//-----------------------------------------------------------------------------
QRectF pqAnimationTrack::boundingRect() const
{
  return this->Rect;
}

//-----------------------------------------------------------------------------
void pqAnimationTrack::setBoundingRect(const QRectF& r)
{
  this->removeFromIndex();
  this->Rect = r;
  this->addToIndex();
  this->update();
}

//-----------------------------------------------------------------------------
pqAnimationKeyFrame* pqAnimationTrack::addKeyFrame()
{
  pqAnimationKeyFrame* frame = new pqAnimationKeyFrame(this);
  this->Frames.append(frame);
  this->update();
  return frame;
}

//-----------------------------------------------------------------------------
void pqAnimationTrack::removeKeyFrame(pqAnimationKeyFrame* frame)
{
  int idx = this->Frames.indexOf(frame);
  if (idx >= 0)
  {
    delete this->Frames.takeAt(idx);
    this->update();
  }
}

//-----------------------------------------------------------------------------
QVariant pqAnimationTrack::property() const
{
  return this->Property;
}

//-----------------------------------------------------------------------------
void pqAnimationTrack::setProperty(const QVariant& p)
{
  this->Property = p;
  Q_EMIT this->propertyChanged();
  this->update();
}

//-----------------------------------------------------------------------------
void pqAnimationTrack::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget* widget)
{
  // draw border for this track
  p->save();
  p->setBrush(Qt::NoBrush);
  QPen pen(widget->palette().color(QPalette::Text));
  pen.setWidth(0);
  p->setPen(pen);
  p->drawRect(this->boundingRect());
  p->restore();
}
