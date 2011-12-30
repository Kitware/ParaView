/*=========================================================================

   Program: ParaView
   Module:    pqTransferFunctionChartViewWidget.h

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
#ifndef __pqTransferFunctionChartViewWidget_h
#define __pqTransferFunctionChartViewWidget_h

#include "pqComponentsExport.h"
#include <QWidget>
#include <QPointer>

class vtkSMSourceProxy;
class pqDataRepresentation;

class vtkColorTransferFunction;
class vtkControlPointsItem;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkChartXY;
class vtkContextScene;
class vtkPlot;
class vtkObject;
class vtkControlPointsItem;
class QVTKWidget;
class QMouseEvent;

/// TransferFunction chart view widget
class PQCOMPONENTS_EXPORT pqTransferFunctionChartViewWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqTransferFunctionChartViewWidget(QWidget* parent=NULL);

  virtual ~pqTransferFunctionChartViewWidget();

  virtual void addPlot(vtkPlot* plot);

  vtkPlot* addLookupTable(vtkLookupTable* lut);
  vtkPlot* addColorTransferFunction(vtkColorTransferFunction* colorTF, bool editable = true);
  vtkPlot* addOpacityFunction(vtkPiecewiseFunction* opacityTF, bool editable = true);
  vtkPlot* addCompositeFunction(vtkColorTransferFunction* colorTF,
    vtkPiecewiseFunction* opacityTF,
    bool colorTFEditable = true,
    bool opacityTFEditable = true);
  vtkPlot* addPiecewiseFunction(vtkPiecewiseFunction* piecewiseTF, bool editable = true);

  vtkPlot* addColorTransferFunctionControlPoints(vtkColorTransferFunction* colorTF);
  vtkPlot* addOpacityFunctionControlPoints(vtkPiecewiseFunction* opacityTF);
  vtkPlot* addCompositeFunctionControlPoints(vtkColorTransferFunction* colorTF,
    vtkPiecewiseFunction* opacityTF);
  vtkPlot* addPiecewiseFunctionControlPoints(vtkPiecewiseFunction* piecewiseTF);

  QList<vtkPlot*> plots()const;
  template<class T>
  QList<T*> plots()const;
  QList<vtkControlPointsItem*> controlPointsItems()const;
  QList<vtkPlot*> lookupTablePlots()const;
  QList<vtkPlot*> lookupTablePlots(vtkLookupTable* lut)const;
  QList<vtkPlot*> colorTransferFunctionPlots()const;
  QList<vtkPlot*> colorTransferFunctionPlots(vtkColorTransferFunction* colorTF)const;
  QList<vtkPlot*> opacityFunctionPlots()const;
  QList<vtkPlot*> opacityFunctionPlots(vtkPiecewiseFunction* opacityTF)const;

  void setLookuptTableToPlots(vtkLookupTable* lut);
  void setColorTransferFunctionToPlots(vtkColorTransferFunction* colorTF);
  void setOpacityFunctionToPlots(vtkPiecewiseFunction* opacityTF);
  void setPiecewiseFunctionToPlots(vtkPiecewiseFunction* piecewiseTF);

  bool bordersVisible()const;
  void setBordersVisible(bool show);

  void validBounds(double bounds[4])const;
  void setValidBounds(double bounds[4]);
  void setPlotsUserBounds(double* bounds);

  /// Return the chart bounds for the 4 chart axes.
  /// bounds must be an array of 8 doubles.
  /// If no bounds is provided by the user, compute the bounds for the 4 chart
  /// axes from the vtkPlots bounds.
  void chartBounds(double* bounds)const;
  void setChartUserBounds(double* bounds);
  void chartUserBounds(double* bounds)const;

  /// Set the chart axes to chart bounds
  virtual void setAxesToChartBounds();

  /// Title that appears inside the view
  QString title()const;
  void setTitle(const QString& title);

  /// clear all plots
  void clearPlots();
  vtkControlPointsItem* currentControlPointsItem();
  QVTKWidget* chartWidget();
  /// Utility function that returns the view chart. It can be used for customizing
  /// the chart display options (axes, legend...)
  vtkChartXY* chart()const;

signals:
    void plotAdded(vtkPlot* plot);
    void currentPointEdited();

public slots:
    void editPoint();
    void resetView();
    void renderView();

protected:

  void chartBoundsToPlotBounds(double bounds[8], double plotBounds[4])const;

  vtkContextScene* scene()const;

private:
  pqTransferFunctionChartViewWidget(const pqTransferFunctionChartViewWidget&); // Not implemented.
  void operator=(const pqTransferFunctionChartViewWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
