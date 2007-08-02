/*=========================================================================

   Program: ParaView
   Module:    pqAnimationModel.h

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

#ifndef pqAnimationModel_h
#define pqAnimationModel_h

#include "QtWidgetsExport.h"

#include <QObject>
#include <QGraphicsScene>
#include <QStandardItemModel>

class pqAnimationTrack;
class QGraphicsView;

// represents a track
class QTWIDGETS_EXPORT pqAnimationModel : public QGraphicsScene
{
  Q_OBJECT
  Q_ENUMS(ModeType)
  Q_PROPERTY(ModeType mode READ mode WRITE setMode)
  Q_PROPERTY(int frames READ frames WRITE setFrames)
  Q_PROPERTY(double currentTime READ currentTime WRITE setCurrentTime)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
public:

  enum ModeType
    {
    Real,
    Sequence,
    TimeSteps
    };

  pqAnimationModel(QGraphicsView* p = 0);
  ~pqAnimationModel();
  
  int count();
  pqAnimationTrack* track(int);

  pqAnimationTrack* addTrack();
  void removeTrack(pqAnimationTrack* track);

  ModeType mode() const;
  int frames() const;
  double currentTime() const;
  double startTime() const;
  double endTime() const;

  QAbstractItemModel* header();
  void setRowHeight(int);
  int rowHeight() const;

public slots:
  void setMode(ModeType);
  void setFrames(int);
  void setCurrentTime(double);
  void setStartTime(double);
  void setEndTime(double);

signals:
  // emitted when a track is double clicked on
  void trackSelected(pqAnimationTrack*);

protected slots:

  void resizeTracks();
  void trackNameChanged();

protected:
  void drawForeground(QPainter* painter, const QRectF& rect);

  bool eventFilter(QObject* w, QEvent* e);

  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);

private:

  ModeType Mode;
  int    Frames;
  double CurrentTime;
  double StartTime;
  double EndTime;
  int    RowHeight;

  QList<pqAnimationTrack*> Tracks;

  // model that provides names of tracks
  QStandardItemModel Header;
};

#endif // pqAnimationModel_h

