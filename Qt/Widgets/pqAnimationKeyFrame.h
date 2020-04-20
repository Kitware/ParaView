/*=========================================================================

   Program: ParaView
   Module:    pqAnimationKeyFrame.h

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

#ifndef pqAnimationKeyFrame_h
#define pqAnimationKeyFrame_h

#include "pqWidgetsModule.h"

#include <QGraphicsItem>
#include <QIcon>
#include <QObject>
class pqAnimationTrack;

// represents a key frame
class PQWIDGETS_EXPORT pqAnimationKeyFrame : public QObject, public QGraphicsItem
{
  Q_OBJECT
  /**
  * the time as a fraction of scene time that this keyframe starts at
  */
  Q_PROPERTY(double normalizedStartTime READ normalizedStartTime WRITE setNormalizedStartTime)
  /**
  * the time as a fraction of scene time that this keyframe ends at
  */
  Q_PROPERTY(double normalizedEndTime READ normalizedEndTime WRITE setNormalizedEndTime)
  /**
  * the value at the start of the keyframe
  */
  Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
  /**
  * the value at the end of the keyframe
  */
  Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
  /**
  * an icon to help describe the keyframe
  */
  Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
  Q_INTERFACES(QGraphicsItem)
public:
  pqAnimationKeyFrame(pqAnimationTrack* p);
  ~pqAnimationKeyFrame() override;

  double normalizedStartTime() const;
  double normalizedEndTime() const;
  QVariant startValue() const;
  QVariant endValue() const;
  QIcon icon() const;

  QRectF boundingRect() const override;

public Q_SLOTS:
  void setNormalizedStartTime(double t);
  void setNormalizedEndTime(double t);
  void setStartValue(const QVariant&);
  void setEndValue(const QVariant&);
  void setIcon(const QIcon& icon);
  void setBoundingRect(const QRectF& r);
  void adjustRect();

Q_SIGNALS:
  void startValueChanged();
  void endValueChanged();
  void iconChanged();

protected:
  /**
  * Returns the parent pqAnimationTrack.
  */
  pqAnimationTrack* parentTrack() const;

  void paint(QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  double NormalizedStartTime;
  double NormalizedEndTime;
  QVariant StartValue;
  QVariant EndValue;
  QIcon Icon;

  QRectF Rect;
};

#endif // pqAnimationKeyFrame_h
