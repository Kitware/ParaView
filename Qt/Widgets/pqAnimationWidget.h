/*=========================================================================

   Program: ParaView
   Module:    pqAnimationWidget.h

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

#ifndef pqAnimationWidget_h
#define pqAnimationWidget_h

#include "QtWidgetsExport.h"

#include <QScrollArea>

class QGraphicsView;
class QHeaderView;
class pqAnimationModel;
class pqAnimationTrack;

class QTWIDGETS_EXPORT pqAnimationWidget : public QScrollArea
{
  Q_OBJECT
public:
  pqAnimationWidget(QWidget* p = 0);
  ~pqAnimationWidget();

  pqAnimationModel* animationModel() const;

  QHeaderView* header() const
    { return this->Header; }

signals:
  // emitted when a track is double clicked on
  void trackSelected(pqAnimationTrack*);

protected slots:
  void updateSizes();
  void headerDblClicked(int);

private:
  QGraphicsView* View;
  QHeaderView* Header;
  pqAnimationModel* Model;

};

#endif //pqAnimationWidget_h

