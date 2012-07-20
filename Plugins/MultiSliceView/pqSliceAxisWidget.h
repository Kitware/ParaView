/*=========================================================================

   Program: ParaView
   Module:  pqSliceAxisWidget.h

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

========================================================================*/
#ifndef __pqSliceAxisWidget_h
#define __pqSliceAxisWidget_h

#include <QWidget>
#include <QPointer>

class vtkChartXY;
class vtkContextScene;
class vtkPlot;
class vtkObject;
class vtkControlPointsItem;
class QVTKWidget;
class QMouseEvent;

class pqSliceAxisWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqSliceAxisWidget(QWidget* parent=NULL);

  virtual ~pqSliceAxisWidget();

  void setRange(double min, double max);

  /// Axis::LEFT=0, Axis::BOTTOM, Axis::RIGHT, Axis::TOP
  void setAxisType(int type);

  /// Title that appears inside the view
  QString title()const;
  void setTitle(const QString& title);

  QVTKWidget* getVTKWidget();

  /// Return the locations of the visible slices
  const double* getVisibleSlices(int &nbSlices) const;

public slots:
  void renderView();

signals:
  void modelUpdated();

protected:
  vtkContextScene* scene() const;
  void invalidateCallback(vtkObject*, unsigned long, void*);

private:
  pqSliceAxisWidget(const pqSliceAxisWidget&); // Not implemented.
  void operator=(const pqSliceAxisWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
