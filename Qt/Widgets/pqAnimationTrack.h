/*=========================================================================

   Program: ParaView
   Module:    pqAnimationTrack.h

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

#ifndef pqAnimationTrack_h
#define pqAnimationTrack_h

#include "pqWidgetsModule.h"

#include <QGraphicsItem>
#include <QList>
#include <QObject>

class pqAnimationKeyFrame;

// represents a track
class PQWIDGETS_EXPORT pqAnimationTrack : public QObject, public QGraphicsItem
{
  Q_OBJECT
/**
* Declare the interfaces implemented - fails with Qt 4.5, warns on 4.6
*/
#if QT_VERSION >= 0x40600
  Q_INTERFACES(QGraphicsItem)
#endif
  /**
  * the property animated in this track
  */
  Q_PROPERTY(QVariant property READ property WRITE setProperty)
public:
  pqAnimationTrack(QObject* p = 0);
  ~pqAnimationTrack();

  /**
  * number of keyframes
  */
  int count();
  /**
  * get a keyframe
  */
  pqAnimationKeyFrame* keyFrame(int);

  /**
  * add a keyframe
  */
  pqAnimationKeyFrame* addKeyFrame();
  /**
  * remove a keyframe
  */
  void removeKeyFrame(pqAnimationKeyFrame* frame);

  bool isDeletable() const { return this->Deletable; }
  void setDeletable(bool d) { this->Deletable = d; }

  QVariant property() const;

  QRectF boundingRect() const;

public slots:
  void setProperty(const QVariant& p);

  void setBoundingRect(const QRectF& r);

  void setEnabled(bool enable)
  {
    this->QGraphicsItem::setEnabled(enable);
    emit this->enabledChanged();
  }

signals:
  void propertyChanged();
  void enabledChanged();

protected:
  void adjustKeyFrameRects();

  virtual void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget);

private:
  bool Deletable;
  QList<pqAnimationKeyFrame*> Frames;
  QVariant Property;

  QRectF Rect;
};

#endif // pqAnimationTrack_h
