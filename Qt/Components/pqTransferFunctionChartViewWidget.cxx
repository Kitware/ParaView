/*=========================================================================

   Program: ParaView
   Module:    pqTransferFunctionChartViewWidget.cxx

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
#include "pqTransferFunctionChartViewWidget.h"

// PV includes
#include "QVTKWidget.h"
#include "vtkSMProperty.h"
#include "vtkSMContextViewProxy.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"

// VTK includes
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkColorTransferControlPointsItem.h"
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkCompositeControlPointsItem.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkLookupTableItem.h"
#include "vtkPen.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkNew.h"

// Qt includes
#include <QColorDialog>
#include <QMouseEvent>
#include <QPalette>
#include <QVBoxLayout>

//----------------------------------------------------------------------------
class pqTransferFunctionChartViewWidget::pqInternal
{

public:
  pqInternal(
    pqTransferFunctionChartViewWidget& object):Widget_ptr(&object)
    {
    this->ContextView = vtkSmartPointer<vtkContextView>::New();
    this->Chart = vtkSmartPointer<vtkChartXY>::New();
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->ChartView = new QVTKWidget(Widget_ptr);

    this->UserBounds[0] = this->UserBounds[2] = this->UserBounds[4] = this->UserBounds[6] = 0.;
    this->UserBounds[1] = this->UserBounds[3] = this->UserBounds[5] = this->UserBounds[7] = -1.;
    this->OldBounds[0] = this->OldBounds[2] = this->OldBounds[4] = this->OldBounds[6] = 0.;
    this->OldBounds[1] = this->OldBounds[3] = this->OldBounds[5] = this->OldBounds[7] = -1.;
    this->ValidBounds[0] = this->ValidBounds[2] = 0.;
    this->ValidBounds[1] = this->ValidBounds[3] = -1.;
    }
  ~pqInternal()
    {
    this->VTKConnect->Disconnect();
    }
  void init()
    {
    this->Chart->SetAutoSize(true);
    this->ContextView->GetScene()->AddItem(this->Chart);
    this->ContextView->SetInteractor(this->ChartView->GetInteractor());
    this->ChartView->SetRenderWindow(this->ContextView->GetRenderWindow());

#ifdef Q_WS_WIN
    this->ContextView->GetRenderWindow()->SetLineSmoothing(true);
#endif
    this->ChartView->setAutomaticImageCacheEnabled(true);

    vtkChartXY* chart = this->Chart;
    chart->SetAutoAxes(false);
    chart->SetHiddenAxisBorder(0);
    chart->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::MIDDLE_BUTTON);
    chart->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::RIGHT_BUTTON);
    }

  void showBorders(bool visible)
    {
    vtkChartXY* chart = this->Chart;
    for (int i = 0; i < 4; ++i)
      {
      chart->GetAxis(i)->SetVisible(visible);
      chart->GetAxis(i)->GetPen()->SetOpacityF(0.3);
      chart->GetAxis(i)->SetNumberOfTicks(0);
      chart->GetAxis(i)->SetBehavior(2);
      chart->GetAxis(i)->SetLabelsVisible(false);
      chart->GetAxis(i)->SetMargins(1, 1);
      chart->GetAxis(i)->SetTitle("");
      }
    }

  void chartBounds(double* bounds)const
    {
    bounds[0] = bounds[2] = bounds[4] = bounds[6] = VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = bounds[7] = VTK_DOUBLE_MIN;
    vtkChartXY* chart = this->Chart;
    const vtkIdType plotCount = chart->GetNumberOfPlots();
    for (vtkIdType i = 0; i < plotCount; ++i)
      {
      vtkPlot* plot = chart->GetPlot(i);

      int corner = chart->GetPlotCorner(plot);
      double plotBounds[4];
      plot->GetBounds(plotBounds);
      switch (corner)
        {
        // bottom left
        case 0:
          // x
          bounds[2] = bounds[2] > plotBounds[0] ?
            plotBounds[0] : bounds[2];
          bounds[3] = bounds[3] < plotBounds[1] ?
            plotBounds[1] : bounds[3];
          // y
          bounds[0] = bounds[0] > plotBounds[2] ?
            plotBounds[2] : bounds[0];
          bounds[1] = bounds[1] < plotBounds[3] ?
            plotBounds[3] : bounds[1];
          break;
          // bottom right
        case 1:
          // x
          bounds[2] = bounds[2] > plotBounds[0] ?
            plotBounds[0] : bounds[2];
          bounds[3] = bounds[3] < plotBounds[1] ?
            plotBounds[1] : bounds[3];
          // y
          bounds[4] = bounds[4] > plotBounds[2] ?
            plotBounds[2] : bounds[4];
          bounds[5] = bounds[5] < plotBounds[3] ?
            plotBounds[3] : bounds[5];
          break;
          // top right
        case 2:
          // x
          bounds[6] = bounds[6] > plotBounds[0] ?
            plotBounds[0] : bounds[6];
          bounds[7] = bounds[7] < plotBounds[1] ?
            plotBounds[1] : bounds[7];
          // y
          bounds[4] = bounds[4] > plotBounds[2] ?
            plotBounds[2] : bounds[4];
          bounds[5] = bounds[5] < plotBounds[3] ?
            plotBounds[3] : bounds[5];
          break;
          // top left
        case 3:
          // x
          bounds[6] = bounds[6] > plotBounds[0] ?
            plotBounds[0] : bounds[6];
          bounds[7] = bounds[7] < plotBounds[1] ?
            plotBounds[1] : bounds[7];
          // y
          bounds[0] = bounds[0] > plotBounds[2] ?
            plotBounds[2] : bounds[1];
          bounds[1] = bounds[0] < plotBounds[3] ?
            plotBounds[3] : bounds[1];
          break;
        }
      }
    }

  vtkSmartPointer<vtkContextView> ContextView;
  vtkSmartPointer<vtkChartXY> Chart;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<QVTKWidget> ChartView;
  double UserBounds[8];
  mutable double OldBounds[8];

  double ValidBounds[4];
  pqTransferFunctionChartViewWidget* Widget_ptr;
  vtkSmartPointer<vtkControlPointsItem> CurrentControlPointsItem;
};

// ----------------------------------------------------------------------------
template<class T>
QList<T*> pqTransferFunctionChartViewWidget::plots()const
{
  QList<T*> res;
  const vtkIdType count = this->chart()->GetNumberOfPlots();
  for(vtkIdType i = 0; i < count; ++i)
    {
    vtkPlot* plot = this->chart()->GetPlot(i);
    if (T::SafeDownCast(plot) != 0)
      {
      res << T::SafeDownCast(plot);
      }
    }
  return res;
}
//-----------------------------------------------------------------------------
pqTransferFunctionChartViewWidget::pqTransferFunctionChartViewWidget(
                             QWidget* parentWidget/*=NULL*/):Superclass(parentWidget)
{
  this->Internal = new pqTransferFunctionChartViewWidget::pqInternal(*this);
  this->Internal->init();
  QVBoxLayout* vLayout = new QVBoxLayout(this);
  vLayout->setMargin(0);
  vLayout->addWidget(this->Internal->ChartView);
}

//-----------------------------------------------------------------------------
pqTransferFunctionChartViewWidget::~pqTransferFunctionChartViewWidget()
{
  this->clearPlots();
  if(this->Internal)
    {
    delete this->Internal;
    }
}
// ----------------------------------------------------------------------------
QVTKWidget* pqTransferFunctionChartViewWidget::chartWidget()
{
  return this->Internal->ChartView;
}
// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::addPlot(vtkPlot* plot)
{
  this->Internal->Chart->AddPlot(plot);
  if(vtkControlPointsItem* currenItem=
    vtkControlPointsItem::SafeDownCast(plot))
    {
    this->Internal->VTKConnect->Disconnect();
    this->Internal->CurrentControlPointsItem =currenItem;
    this->Internal->VTKConnect->Connect(
      currenItem, vtkControlPointsItem::CurrentPointEditEvent,
      this, SLOT(editPoint()));
    }
  emit this->plotAdded(plot);
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget::addLookupTable(vtkLookupTable* lut)
{
  vtkSmartPointer<vtkLookupTableItem> item =
    vtkSmartPointer<vtkLookupTableItem>::New();
  item->SetLookupTable(lut);
  this->addPlot(item);
  return item;
}
// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addColorTransferFunction(vtkColorTransferFunction* colorTF,
                           bool editable)
{
  vtkSmartPointer<vtkColorTransferFunctionItem> item =
    vtkSmartPointer<vtkColorTransferFunctionItem>::New();
  item->SetColorTransferFunction(colorTF);
  this->addPlot(item);
  if (editable)
    {
    this->addColorTransferFunctionControlPoints(colorTF);
    }
  return item;
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addOpacityFunction(vtkPiecewiseFunction* opacityTF,
                     bool editable)
{
  return this->addPiecewiseFunction(opacityTF, editable);
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addPiecewiseFunction(vtkPiecewiseFunction* piecewiseTF,
                       bool editable)
{
  vtkSmartPointer<vtkPiecewiseFunctionItem> item =
    vtkSmartPointer<vtkPiecewiseFunctionItem>::New();
  item->SetPiecewiseFunction(piecewiseTF);
  QColor defaultColor = this->Internal->ChartView->palette().highlight().color();
  item->SetColor(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF());
  item->SetMaskAboveCurve(true);
  this->addPlot(item);
  if (editable)
    {
    this->addPiecewiseFunctionControlPoints(piecewiseTF);
    }
  return item;
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget::addCompositeFunction(vtkColorTransferFunction* colorTF,
                       vtkPiecewiseFunction* opacityTF,
                       bool colorTFEditable, bool opacityTFEditable)
{
  vtkSmartPointer<vtkCompositeTransferFunctionItem> item =
    vtkSmartPointer<vtkCompositeTransferFunctionItem>::New();
  item->SetColorTransferFunction(colorTF);
  item->SetOpacityFunction(opacityTF);
  item->SetMaskAboveCurve(true);
  this->addPlot(item);
  if (colorTFEditable && opacityTFEditable)
    {
    this->addCompositeFunctionControlPoints(colorTF, opacityTF);
    }
  else if (colorTFEditable)
    {
    this->addColorTransferFunctionControlPoints(colorTF);
    }
  else if (opacityTFEditable)
    {
    this->addOpacityFunctionControlPoints(opacityTF);
    }
  return item;
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addColorTransferFunctionControlPoints(vtkColorTransferFunction* colorTF)
{
  vtkNew<vtkColorTransferControlPointsItem> controlPointsItem;
  controlPointsItem->SetColorTransferFunction(colorTF);
  controlPointsItem->SetColorFill(true);
  controlPointsItem->SetValidBounds(this->Internal->ValidBounds);
  controlPointsItem->SetEndPointsXMovable(false);
  controlPointsItem->SetEndPointsYMovable(false);
  controlPointsItem->SetEndPointsRemovable(false);

  this->addPlot(controlPointsItem.GetPointer());
  return controlPointsItem.GetPointer();
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addOpacityFunctionControlPoints(vtkPiecewiseFunction* opacityTF)
{
  return this->addPiecewiseFunctionControlPoints(opacityTF);
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addCompositeFunctionControlPoints(vtkColorTransferFunction* colorTF,
                                    vtkPiecewiseFunction* opacityTF)
{
  vtkNew<vtkCompositeControlPointsItem> controlPointsItem;
  controlPointsItem->SetColorTransferFunction(colorTF);
  controlPointsItem->SetOpacityFunction(opacityTF);
  controlPointsItem->SetValidBounds(this->Internal->ValidBounds);
  controlPointsItem->SetEndPointsXMovable(false);
  controlPointsItem->SetUseOpacityPointHandles(true);
  controlPointsItem->SetEndPointsRemovable(false);
  this->addPlot(controlPointsItem.GetPointer());
  return controlPointsItem.GetPointer();
}

// ----------------------------------------------------------------------------
vtkPlot* pqTransferFunctionChartViewWidget
::addPiecewiseFunctionControlPoints(vtkPiecewiseFunction* piecewiseTF)
{
  vtkNew<vtkPiecewiseControlPointsItem> controlPointsItem;
  controlPointsItem->SetPiecewiseFunction(piecewiseTF);
  controlPointsItem->SetValidBounds(this->Internal->ValidBounds);
  controlPointsItem->SetEndPointsRemovable(false);
  this->addPlot(controlPointsItem.GetPointer());
  return controlPointsItem.GetPointer();
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget::plots()const
{
  QList<vtkPlot*> res;
  const vtkIdType count = this->chart()->GetNumberOfPlots();
  for(vtkIdType i = 0; i < count; ++i)
    {
    res << this->chart()->GetPlot(i);
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkControlPointsItem*> pqTransferFunctionChartViewWidget
::controlPointsItems()const
{
  QList<vtkControlPointsItem*> res;
  foreach(vtkPlot* plot, this->plots())
    {
    vtkControlPointsItem* controlPointsItem =
      vtkControlPointsItem::SafeDownCast(plot);
    if (controlPointsItem)
      {
      res << controlPointsItem;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget::lookupTablePlots()const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->plots())
    {
    if (vtkLookupTableItem::SafeDownCast(plot))
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget::lookupTablePlots(vtkLookupTable* lut)const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->lookupTablePlots())
    {
    vtkLookupTableItem* item = vtkLookupTableItem::SafeDownCast(plot);
    if (item->GetLookupTable() == lut)
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget::colorTransferFunctionPlots()const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->plots())
    {
    if (vtkColorTransferFunctionItem::SafeDownCast(plot) ||
        vtkColorTransferControlPointsItem::SafeDownCast(plot))
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget
::colorTransferFunctionPlots(vtkColorTransferFunction* colorTF)const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->colorTransferFunctionPlots())
    {
    vtkColorTransferFunctionItem* item =
      vtkColorTransferFunctionItem::SafeDownCast(plot);
    if (item
        && item->GetColorTransferFunction() == colorTF)
      {
      res << plot;
      }
    vtkColorTransferControlPointsItem* controlPointsItem =
      vtkColorTransferControlPointsItem::SafeDownCast(plot);
    if (controlPointsItem
        && controlPointsItem->GetColorTransferFunction() == colorTF)
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget::opacityFunctionPlots()const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->plots())
    {
    if (vtkPiecewiseFunctionItem::SafeDownCast(plot) ||
        vtkPiecewiseControlPointsItem::SafeDownCast(plot) ||
        vtkCompositeTransferFunctionItem::SafeDownCast(plot) ||
        vtkCompositeControlPointsItem::SafeDownCast(plot))
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
QList<vtkPlot*> pqTransferFunctionChartViewWidget
::opacityFunctionPlots(vtkPiecewiseFunction* opacityTF)const
{
  QList<vtkPlot*> res;
  foreach(vtkPlot* plot, this->opacityFunctionPlots())
    {
    vtkPiecewiseFunctionItem* item =
      vtkPiecewiseFunctionItem::SafeDownCast(plot);
    if (item
        && item->GetPiecewiseFunction() == opacityTF)
      {
      res << plot;
      }
    vtkPiecewiseControlPointsItem* controlPointsItem =
      vtkPiecewiseControlPointsItem::SafeDownCast(plot);
    if (controlPointsItem
        && controlPointsItem->GetPiecewiseFunction() == opacityTF)
      {
      res << plot;
      }
    vtkCompositeTransferFunctionItem* compositeItem =
      vtkCompositeTransferFunctionItem::SafeDownCast(plot);
    if (compositeItem
        && compositeItem->GetOpacityFunction() == opacityTF)
      {
      res << plot;
      }
    vtkCompositeControlPointsItem* compositeControlPointsItem =
      vtkCompositeControlPointsItem::SafeDownCast(plot);
    if (compositeControlPointsItem
        && compositeControlPointsItem->GetOpacityFunction() == opacityTF)
      {
      res << plot;
      }
    }
  return res;
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::setLookuptTableToPlots(vtkLookupTable* lut)
{
  foreach(vtkLookupTableItem* plot,
          this->plots<vtkLookupTableItem>())
    {
    plot->SetLookupTable(lut);
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setColorTransferFunctionToPlots(vtkColorTransferFunction* colorTF)
{
  foreach(vtkColorTransferFunctionItem* plot,
          this->plots<vtkColorTransferFunctionItem>())
    {
    plot->SetColorTransferFunction(colorTF);
    }
  foreach(vtkColorTransferControlPointsItem* plot,
          this->plots<vtkColorTransferControlPointsItem>())
    {
    plot->SetColorTransferFunction(colorTF);
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setOpacityFunctionToPlots(vtkPiecewiseFunction* opacityTF)
{
  this->setPiecewiseFunctionToPlots(opacityTF);
  foreach(vtkCompositeTransferFunctionItem* plot,
          this->plots<vtkCompositeTransferFunctionItem>())
    {
    plot->SetOpacityFunction(opacityTF);
    }
  foreach(vtkCompositeControlPointsItem* plot,
          this->plots<vtkCompositeControlPointsItem>())
    {
    plot->SetOpacityFunction(opacityTF);
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setPiecewiseFunctionToPlots(vtkPiecewiseFunction* piecewiseTF)
{
  foreach(vtkPiecewiseFunctionItem* plot,
          this->plots<vtkPiecewiseFunctionItem>())
    {
    plot->SetPiecewiseFunction(piecewiseTF);
    }
  foreach(vtkPiecewiseControlPointsItem* plot,
          this->plots<vtkPiecewiseControlPointsItem>())
    {
    plot->SetPiecewiseFunction(piecewiseTF);
    }
}

// ----------------------------------------------------------------------------
bool pqTransferFunctionChartViewWidget
::bordersVisible()const
{
  return this->chart()->GetAxis(0)->GetVisible();
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setBordersVisible(bool visible)
{
  this->Internal->showBorders(visible);
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::validBounds(double* bounds)const
{
  memcpy(bounds, this->Internal->ValidBounds, 4 * sizeof(double));
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setValidBounds(double* bounds)
{
  foreach(vtkControlPointsItem* plot, this->controlPointsItems())
    {
    plot->SetValidBounds(bounds);
    }
  memcpy(this->Internal->ValidBounds, bounds, 4 * sizeof(double));
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget
::setPlotsUserBounds(double* bounds)
{
  double plotBounds[4];
  this->chartBoundsToPlotBounds(bounds, plotBounds);
  foreach(vtkScalarsToColorsItem* plot,
          this->plots<vtkScalarsToColorsItem>())
    {
    plot->SetUserBounds(plotBounds);
    }
  foreach(vtkControlPointsItem* plot, this->controlPointsItems())
    {
    plot->SetUserBounds(plotBounds);
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::editPoint()
{
  vtkControlPointsItem* controlPoints=this->
    currentControlPointsItem();
  if(!controlPoints)
    {
    return;
    }
  int pointToEdit=controlPoints->GetCurrentPoint();
  if (pointToEdit < 0)
    {
    return;
    }
  vtkColorTransferControlPointsItem* colorTransferFunctionItem =
    vtkColorTransferControlPointsItem::SafeDownCast(controlPoints);
  vtkCompositeControlPointsItem* compositeControlPoints =
    vtkCompositeControlPointsItem::SafeDownCast(controlPoints);
  if (colorTransferFunctionItem &&
      (!compositeControlPoints ||
        compositeControlPoints->GetPointsFunction() == vtkCompositeControlPointsItem::ColorPointsFunction ||
        compositeControlPoints->GetPointsFunction() == vtkCompositeControlPointsItem::ColorAndOpacityPointsFunction))
    {
    double xrgbms[6];
    vtkColorTransferFunction* colorTF = colorTransferFunctionItem->GetColorTransferFunction();
    if(!colorTF)
      {
      return;
      }
    colorTF->GetNodeValue(pointToEdit, xrgbms);
    QColor oldColor = QColor::fromRgbF(xrgbms[1], xrgbms[2], xrgbms[3]);
    QColor newColor = QColorDialog::getColor(oldColor, this->Internal->ChartView);
    if (newColor.isValid())
      {
      xrgbms[1] = newColor.redF();
      xrgbms[2] = newColor.greenF();
      xrgbms[3] = newColor.blueF();
      colorTF->SetNodeValue(pointToEdit, xrgbms);
      emit this->currentPointEdited();
      }
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::setTitle(const QString& newTitle)
{
  this->Internal->Chart->SetTitle(newTitle.toLatin1().data());
}

// ----------------------------------------------------------------------------
QString pqTransferFunctionChartViewWidget::title()const
{
  return QString(this->Internal->Chart->GetTitle());
}
// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::clearPlots()
{
  this->blockSignals(true);
  this->setLookuptTableToPlots(0);
  if(this->currentControlPointsItem())
    {
    this->currentControlPointsItem()->SetVisible(false);
    }
  this->setOpacityFunctionToPlots(0);
  this->setPiecewiseFunctionToPlots(0);
  this->setColorTransferFunctionToPlots(0);
  this->Internal->VTKConnect->Disconnect();
  this->chart()->ClearPlots();
  this->chart()->ClearItems();
  this->blockSignals(false);
  this->Internal->CurrentControlPointsItem=NULL;
}
// ----------------------------------------------------------------------------
vtkControlPointsItem* pqTransferFunctionChartViewWidget
  ::currentControlPointsItem()
{
  return this->Internal->CurrentControlPointsItem;
}

// ----------------------------------------------------------------------------
vtkChartXY* pqTransferFunctionChartViewWidget::chart()const
{
  return this->Internal->Chart;
}

// ----------------------------------------------------------------------------
vtkContextScene* pqTransferFunctionChartViewWidget::scene()const
{
  return this->Internal->ContextView->GetScene();
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::chartBounds(double* bounds)const
{
  if (this->Internal->UserBounds[1] < this->Internal->UserBounds[0])
    {
    // Invalid user bounds, return the real chart bounds
    this->Internal->chartBounds(bounds);
    }
  else
    {
    this->chartUserBounds(bounds);
    }
  
  memcpy(this->Internal->OldBounds, bounds, 8 * sizeof(double));
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::setChartUserBounds(double* userBounds)
{
  for (int i= 0; i < 8; ++i)
    {
    this->Internal->UserBounds[i] = userBounds[i];
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::chartUserBounds(double* bounds)const
{
  for (int i= 0; i < 8; ++i)
    {
    bounds[i] = this->Internal->UserBounds[i];
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::setAxesToChartBounds()
{
  vtkChartXY* currentchart = this->chart();
  double bounds[8];
  this->chartBounds(bounds);
  for (int i = 0; i < currentchart->GetNumberOfAxes(); ++i)
    {
    if (bounds[2*i] != VTK_DOUBLE_MAX)
      {
      double deltarange=bounds[2*i+1]-bounds[2*i];
      if(deltarange==0)
        {
        double range= 1.0;
        currentchart->GetAxis(i)->SetRange(0,range);
        currentchart->GetAxis(i)->SetBehavior(2);
        }
      else
        {
        deltarange *= (deltarange <=1) ? 0.05 : 0.02;
        currentchart->GetAxis(i)->SetRange(bounds[2*i]-deltarange,
          bounds[2*i+1]+deltarange);
        currentchart->GetAxis(i)->SetBehavior(2);
        }
      }
    }
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::chartBoundsToPlotBounds(double bounds[8], double plotBounds[4])const
{
  plotBounds[0] = bounds[vtkAxis::BOTTOM*2];
  plotBounds[1] = bounds[vtkAxis::BOTTOM*2 + 1];
  plotBounds[2] = bounds[vtkAxis::LEFT*2];
  plotBounds[3] = bounds[vtkAxis::LEFT*2+1];
}

// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::resetView()
{
  this->Internal->Chart->RecalculateBounds();
  this->setAxesToChartBounds();
  this->setBordersVisible(true);
  this->renderView();
}
// ----------------------------------------------------------------------------
void pqTransferFunctionChartViewWidget::renderView()
{
  this->Internal->ChartView->GetRenderWindow()->Render();
}
