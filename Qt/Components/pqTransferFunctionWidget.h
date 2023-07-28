// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTransferFunctionWidget_h
#define pqTransferFunctionWidget_h

#include "pqComponentsModule.h"
#include "vtkType.h"
#include <QWidget>

class vtkPiecewiseFunction;
class vtkScalarsToColors;
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
  pqTransferFunctionWidget(QWidget* parent = nullptr);
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

  ///@{
  /**
   * Switches the chart to use a log scaled X axis.
   */
  void SetLogScaleXAxis(bool logScale);
  bool GetLogScaleXAxis() const;
  ///@}

  ///@{
  /**
   * Provides access to vtkScalarsToColors and vtkPiecewiseFunction passed to
   * `initialize`.
   */
  vtkScalarsToColors* scalarsToColors() const;
  vtkPiecewiseFunction* piecewiseFunction() const;
  ///@}

  ///@{
  /**
   * Set/Get the use of freehand drawing for the control points.
   */
  void SetControlPointsFreehandDrawing(bool use);
  bool GetControlPointsFreehandDrawing() const;
  ///@}

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
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

Q_SIGNALS:
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

  /**
   * signal fired when the range handles changed the range.
   */
  void rangeHandlesRangeChanged(double rangeMin, double rangeMax);

  /**
   * signal fired when the range handles are double clicked.
   */
  void rangeHandlesDoubleClicked();

protected Q_SLOTS:
  /**
   * slot called when the internal vtkControlPointsItem fires
   * vtkControlPointsItem::CurrentPointChangedEvent
   */
  void onCurrentChangedEvent();

  /**
   * slot called when the internal vtkRangeHandlesItem fires a
   * vtkRangeHandlesItem::RangeHandlesRangeChanged
   */
  void onRangeHandlesRangeChanged();

  /**
   * Show usage info in the application status bar
   */
  void showUsageStatus();

  /**
   * Show color dialog to edit color for a specific control point.
   */
  void editColorAtCurrentControlPoint();

protected: // NOLINT(readability-redundant-access-specifiers)
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
