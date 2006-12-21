/*=========================================================================

   Program: ParaView
   Module:    pqColorMapWidget.h

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

/// \file pqColorMapWidget.h
/// \date 7/7/2006

#ifndef _pqColorMapWidget_h
#define _pqColorMapWidget_h


#include "QtChartExport.h"
#include <QAbstractScrollArea>

class pqChartValue;
class pqColorMapWidgetInternal;
class QColor;
class QPixmap;


/// \class pqColorMapWidget
class QTCHART_EXPORT pqColorMapWidget : public QAbstractScrollArea
{
  Q_OBJECT

public:
  pqColorMapWidget(QWidget *parent=0);
  virtual ~pqColorMapWidget();

  virtual QSize sizeHint() const;

  int getPointCount() const;
  void addPoint(const pqChartValue &value, const QColor &color);
  void removePoint(int index);
  void clearPoints();

  void getPointValue(int index, pqChartValue &value) const;
  void getPointColor(int index, QColor &color) const;
  void setPointColor(int index, const QColor &color);

  void setTableSize(int tableSize);
  int getTableSize() const {return this->TableSize;}

  void setMovingPointsAllowed(bool allowed) {this->MovingAllowed = allowed;}
  bool isMovingPointsAllowed() const {return this->MovingAllowed;}
  void setAddingPointsAllowed(bool allowed) {this->AddingAllowed = allowed;}
  bool isAddingPointsAllowed() const {return this->AddingAllowed;}

  void layoutColorMap();

signals:
  void colorChangeRequested(int index);
  void colorChanged(int index, const QColor &color);

protected:
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseDoubleClickEvent(QMouseEvent *e);
  virtual void paintEvent(QPaintEvent *e);
  virtual void resizeEvent(QResizeEvent *e);
  virtual void scrollContentsBy(int dx, int dy);

private:
  pqColorMapWidgetInternal *Internal; ///< Stores the color map data.
  QPixmap *DisplayImage;              ///< Used for drawing color map.
  int TableSize;                      ///< Stores the color table size.
  int Spacing;                        ///< Stores the layout spacing.
  int Margin;                         ///< Stores the layout margin.
  int PointWidth;                     ///< Stores the size of the points.
  bool AddingAllowed;                 ///< True if points can be added.
  bool MovingAllowed;                 ///< True if points can be moved.
};

#endif
