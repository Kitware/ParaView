/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqTransferFunctionWidget_h
#define pqTransferFunctionWidget_h

#include "pqComponentsModule.h"
#include "vtkType.h"
#include <QWidget>

class vtkScalarsToColors;
class vtkPiecewiseFunction;
class vtkTable;

/**
* pqTransferFunctionWidget provides a widget that can edit the control points
* in a vtkPiecewiseFunction or a vtkScalarsToColors (or subclass) or both.
* It is used by pqColorOpacityEditorWidget, for example, to show transfer
* function editors for color and opacity for a PVLookupTable proxy.
*/
class PQCOMPONENTS_EXPORT pqTransferFunctionWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqTransferFunctionWidget(QWidget* parent = 0);
  ~pqTransferFunctionWidget() override;

  /**
  * Initialize the pqTransferFunctionWidget with the given
  * color-transfer-function and piecewise-function (either of which can be
  * null). The editable flags are used to control if the users should be
  * allowed to edit/change the particular transfer function.
  */
  void initialize(
    vtkScalarsToColors* stc, bool stc_editable, vtkPiecewiseFunction* pwf, bool pwf_editable);

  /**
  * Returns the current point index. -1 is none is selected.
  */
  vtkIdType currentPoint() const;

  /**
  * Returns the number of control points in the editor widget.
  */
  vtkIdType numberOfControlPoints() const;

  /**
   * Switches the chart to use a log scaled X axis.
   */
  void SetLogScaleXAxis(bool logScale);
  bool GetLogScaleXAxis() const;

public slots:
  /**
  * Set the current point. Set to -1 clear the current point.
  */
  void setCurrentPoint(vtkIdType index);

  /**
  * Set the X-position for the current point (without changing the Y position,
  * if applicable. We ensure that the point doesn't move past neighbouring
  * points. Note this will not change the end points i.e. start and end points.
  */
  void setCurrentPointPosition(double xpos);

  /**
  * re-renders the transfer function view. This doesn't render immediately,
  * schedules a render.
  */
  void render();

  /**
   * Set the histogram table to display as a plot bar.
   * If set to nullptr, a simple color texture is used, the default.
   */
  void setHistogramTable(vtkTable* table);

signals:
  /**
  * signal fired when the \c current selected control point changes.
  */
  void currentPointChanged(vtkIdType index);

  /**
  * signal fired to indicate that the control points changed i.e. either they
  * were moved, orone was added/deleted, or edited to change color, etc.
  */
  void controlPointsModified();

  /**
   * signal fired when the chart range is modified.
   */
  void chartRangeModified();

protected slots:
  /**
  * slot called when the internal vtkControlPointsItem fires
  * vtkControlPointsItem::CurrentPointChangedEvent
  */
  void onCurrentChangedEvent();

protected:
  /**
  * callback called when vtkControlPointsItem fires
  * vtkControlPointsItem::CurrentPointEditEvent.
  */
  void onCurrentPointEditEvent();

private:
  Q_DISABLE_COPY(pqTransferFunctionWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
