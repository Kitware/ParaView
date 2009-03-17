/*=========================================================================

   Program: ParaView
   Module:    pqCameraWidget.h

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
#ifndef __pqCameraWidget_h
#define __pqCameraWidget_h

#include <QWidget>
#include <QList>
#include <QVariant>
#include "pqComponentsExport.h"

/// widget class with createors for position, focalpoint, viewup and view angle
class PQCOMPONENTS_EXPORT pqCameraWidget : public QWidget 
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> position READ position WRITE setPosition)
  Q_PROPERTY(QList<QVariant> focalPoint READ focalPoint WRITE setFocalPoint)
  Q_PROPERTY(QList<QVariant> viewUp READ viewUp WRITE setViewUp)
  Q_PROPERTY(QVariant viewAngle READ viewAngle WRITE setViewAngle)

  typedef QWidget Superclass;
public:
  pqCameraWidget(QWidget* parent=NULL);
  virtual ~pqCameraWidget();

signals:
  void positionChanged();
  void focalPointChanged();
  void viewUpChanged();
  void viewAngleChanged();
  void useCurrent();

public slots:
  void setPosition(QList<QVariant>);
  void setFocalPoint(QList<QVariant>);
  void setViewUp(QList<QVariant>);
  void setViewAngle(QVariant);
  void setUsePaths(bool);
  void setFocalPointPath(const QList<QVariant>&);
  void setPositionPath(const QList<QVariant>&);
  void setClosedPositionPath(bool);
  void setClosedFocalPath(bool);

protected slots:
  void createFocalPointPath();
  void createPositionPath();
  void showDialog();
  void hideDialog();

protected:
  // Overridden to update the 3D widget's visibility states.
  virtual void showEvent(QShowEvent*);
  virtual void hideEvent(QHideEvent*);

public:
  QList<QVariant> position() const;
  QList<QVariant> focalPoint() const;
  QList<QVariant> viewUp() const;
  QList<QVariant> focalPath() const;
  QList<QVariant> positionPath() const;
  QVariant viewAngle() const;
  bool usePaths() const;
  bool closedPositionPath() const;
  bool closedFocalPath() const;
private:
  class pqInternal;
  pqInternal* Internal;
};

#endif

