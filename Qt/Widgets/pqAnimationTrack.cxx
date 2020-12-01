/*=========================================================================

   Program: ParaView
   Module:    pqAnimationTrack.cxx

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
  emit this->propertyChanged();
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
