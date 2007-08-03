/*=========================================================================

   Program: ParaView
   Module:    pqCameraWidget.h

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
#ifndef __pqCameraWidget_h
#define __pqCameraWidget_h

#include <QWidget>
#include <QList>
#include <QVariant>
#include "pqComponentsExport.h"

/// widget class with editors for position, focalpoint, viewup and view angle
class PQCOMPONENTS_EXPORT pqCameraWidget : public QWidget 
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> position READ position WRITE setPosition)
  Q_PROPERTY(QList<QVariant> focalPoint READ focalPoint WRITE setFocalPoint)
  Q_PROPERTY(QList<QVariant> viewUp READ viewUp WRITE setViewUp)
  Q_PROPERTY(QVariant viewAngle READ viewAngle WRITE setViewAngle)
public:
  pqCameraWidget(QWidget* parent=NULL);
  virtual ~pqCameraWidget();

signals:
  void positionChanged();
  void focalPointChanged();
  void viewUpChanged();
  void viewAngleChanged();

public slots:
  void setPosition(QList<QVariant>);
  void setFocalPoint(QList<QVariant>);
  void setViewUp(QList<QVariant>);
  void setViewAngle(QVariant);

public:
  QList<QVariant> position() const;
  QList<QVariant> focalPoint() const;
  QList<QVariant> viewUp() const;
  QVariant viewAngle() const;

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif

