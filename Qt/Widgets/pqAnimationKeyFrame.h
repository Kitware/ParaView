/*=========================================================================

   Program: ParaView
   Module:    pqAnimationKeyFrame.h

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

#ifndef pqAnimationKeyFrame_h
#define pqAnimationKeyFrame_h

#include "QtWidgetsExport.h"

#include <QObject>
#include <QGraphicsItem>
class pqAnimationTrack;

// represents a key frame
class QTWIDGETS_EXPORT pqAnimationKeyFrame : public QObject, public QGraphicsItem
{
  Q_OBJECT
  Q_ENUMS(InterpolationType)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
  Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
  Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
  Q_PROPERTY(InterpolationType interpolation READ interpolation
                                             WRITE setInterpolation)
public:

  enum InterpolationType
  {
    Linear
  };

  pqAnimationKeyFrame(pqAnimationTrack* p, QGraphicsScene* s);
  ~pqAnimationKeyFrame();

  double startTime() const;
  double endTime() const;
  QVariant startValue() const;
  QVariant endValue() const;
  InterpolationType interpolation() const;
  
  QRectF boundingRect() const;

public slots:
  void setStartTime(double t);
  void setEndTime(double t);
  void setStartValue(const QVariant&);
  void setEndValue(const QVariant&);
  void setInterpolation(InterpolationType);
  void setBoundingRect(const QRectF& r);
  void adjustRect();

signals:
  void startValueChanged();
  void endValueChanged();
  void interpolationChanged();

protected:

  virtual void paint(QPainter* p,
                     const QStyleOptionGraphicsItem * option,
                     QWidget * widget);
  

private:
  double StartTime;
  double EndTime;
  QVariant StartValue;
  QVariant EndValue;
  InterpolationType Interpolation;

  QRectF Rect;

};

#endif // pqAnimationKeyFrame_h

