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
#include "pqTransferFunctionWidget.h"

#include "QVTKOpenGLNativeWidget.h"
#include "pqCoreUtilities.h"
#include "pqTimer.h"
#include "vtkAxis.h"
#include "vtkBoundingBox.h"
#include "vtkChartXY.h"
#include "vtkColorTransferControlPointsItem.h"
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkCompositeControlPointsItem.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseControlPointsItem.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#include <QColorDialog>
#include <QPointer>
#include <QSurfaceFormat>
#include <QVBoxLayout>

#include <algorithm>
#include <cassert>

namespace
{

//-----------------------------------------------------------------------------
// vtkTransferFunctionChartXY extends vtkChartXY to support following
// customizations:
// 1. The axes should always show the bounds for the transfer function's XY
//    range. Thus, we need to change the axes range whenever the transfer
//    function range changes.
//
// 2. If the transfer function range is too small, disable interaction and
//    potentially rendering of the transfer function since it won't be correct.
//
class vtkTransferFunctionChartXY : public vtkChartXY
{
  vtkVector2d XRange;
  vtkVector2d XRangePrevious;
  bool DataValid;

  bool IsDataRangeValid(const double r[2]) const
  {
    double mr[2] = { r[0], r[1] };
    // If vtkSMCoreUtilities::AdjustRange() decided to adjust a valid range, it means the numbers
    // are too close to each other.
    return r[1] < r[0] ? false : (vtkSMCoreUtilities::AdjustRange(mr) == false);
  }

public:
  static vtkTransferFunctionChartXY* New();
  vtkTypeMacro(vtkTransferFunctionChartXY, vtkChartXY);

  void SetTFRange(const vtkVector2d& range) { this->XRange = range; }

  void Update() override
  {
    this->Superclass::Update();
    if (this->XRange != this->XRangePrevious)
    {
      this->XRangePrevious = this->XRange;
      this->DataValid = this->IsDataRangeValid(this->XRange.GetData());
      this->AdjustAxes();
    }
  }
  bool PaintChildren(vtkContext2D* painter) override
  {
    if (this->DataValid)
    {
      return this->Superclass::PaintChildren(painter);
    }
    painter->DrawString(5, 5, "Data range too small to render.");
    return true;
  }

  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override
  {
    return (this->DataValid ? this->Superclass::MouseEnterEvent(mouse) : false);
  }
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override
  {
    return (this->DataValid ? this->Superclass::MouseMoveEvent(mouse) : false);
  }
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override
  {
    return (this->DataValid ? this->Superclass::MouseLeaveEvent(mouse) : false);
  }
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override
  {
    return (this->DataValid ? this->Superclass::MouseButtonPressEvent(mouse) : false);
  }
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override
  {
    return (this->DataValid ? this->Superclass::MouseButtonReleaseEvent(mouse) : false);
  }
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override
  {
    return (this->DataValid ? this->Superclass::MouseWheelEvent(mouse, delta) : false);
  }
  bool KeyPressEvent(const vtkContextKeyEvent& key) override
  {
    return (this->DataValid ? this->Superclass::KeyPressEvent(key) : false);
  }

protected:
  vtkTransferFunctionChartXY()
  {
    this->XRange = this->XRangePrevious = vtkVector2d(0, 0);
    this->DataValid = false;
  }
  ~vtkTransferFunctionChartXY() override {}

  void AdjustAxes()
  {
    this->GetAxis(vtkAxis::BOTTOM)->SetUnscaledRange(this->XRange[0], this->XRange[1]);
    this->GetAxis(vtkAxis::LEFT)->SetRange(0, 1);

    // for recalculation of transforms using current axes ranges.
    this->RecalculatePlotTransforms();
  }

private:
  vtkTransferFunctionChartXY(const vtkTransferFunctionChartXY&);
  void operator=(const vtkTransferFunctionChartXY&);
};
vtkStandardNewMacro(vtkTransferFunctionChartXY);

} // end of namespace

//-----------------------------------------------------------------------------

class pqTransferFunctionWidget::pqInternals
{
  vtkNew<vtkGenericOpenGLRenderWindow> Window;

public:
  QPointer<QVTKOpenGLNativeWidget> Widget;
  vtkNew<vtkTransferFunctionChartXY> ChartXY;
  vtkNew<vtkContextView> ContextView;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

  pqTimer Timer;

  vtkSmartPointer<vtkScalarsToColorsItem> TransferFunctionItem;
  vtkSmartPointer<vtkControlPointsItem> ControlPointsItem;
  unsigned long CurrentPointEditEventId;

  pqInternals(pqTransferFunctionWidget* editor)
    : Widget(new QVTKOpenGLNativeWidget(editor))
    , CurrentPointEditEventId(0)
  {
    this->Timer.setSingleShot(true);
    this->Timer.setInterval(0);

    this->Window->SetMultiSamples(8);

    this->Widget->setEnableHiDPI(true);
    this->Widget->setObjectName("1QVTKWidget0");
    this->Widget->setRenderWindow(this->Window.Get());
    this->ContextView->SetRenderWindow(this->Window.Get());

    this->ChartXY->SetAutoSize(true);
    this->ChartXY->SetShowLegend(false);
    this->ChartXY->SetForceAxesToBounds(true);
    this->ChartXY->SetZoomWithMouseWheel(false);
    this->ContextView->GetScene()->AddItem(this->ChartXY.GetPointer());
    this->ContextView->SetInteractor(this->Widget->interactor());
    this->ContextView->GetRenderWindow()->SetLineSmoothing(true);

    this->ChartXY->SetActionToButton(vtkChart::PAN, -1);
    this->ChartXY->SetActionToButton(vtkChart::ZOOM, -1);
    this->ChartXY->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::RIGHT_BUTTON);
    this->ChartXY->SetActionToButton(vtkChart::SELECT_POLYGON, -1);

    this->Widget->setParent(editor);
    QVBoxLayout* layout = new QVBoxLayout(editor);
    layout->setMargin(0);
    layout->addWidget(this->Widget);

    this->ChartXY->SetAutoAxes(false);
    this->ChartXY->SetHiddenAxisBorder(8);
    for (int cc = 0; cc < 4; cc++)
    {
      this->ChartXY->GetAxis(cc)->SetVisible(false);
      this->ChartXY->GetAxis(cc)->SetBehavior(vtkAxis::FIXED);
    }
  }
  ~pqInternals() { this->cleanup(); }

  void cleanup()
  {
    this->VTKConnect->Disconnect();
    this->ChartXY->ClearPlots();
    if (this->ControlPointsItem && this->CurrentPointEditEventId)
    {
      this->ControlPointsItem->RemoveObserver(this->CurrentPointEditEventId);
      this->CurrentPointEditEventId = 0;
    }
    this->TransferFunctionItem = NULL;
    this->ControlPointsItem = NULL;
  }
};

//-----------------------------------------------------------------------------
pqTransferFunctionWidget::pqTransferFunctionWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals(this))
{
  // whenever the rendering timer times out, we render the widget.
  QObject::connect(&this->Internals->Timer, &QTimer::timeout, [this]() {
    auto renWin = this->Internals->ContextView->GetRenderWindow();
    if (this->isVisible() && renWin->IsDrawable())
    {
      renWin->Render();
    }
  });
}

//-----------------------------------------------------------------------------
pqTransferFunctionWidget::~pqTransferFunctionWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::initialize(
  vtkScalarsToColors* stc, bool stc_editable, vtkPiecewiseFunction* pwf, bool pwf_editable)
{
  this->Internals->cleanup();

  // TODO: If needed, we can support vtkLookupTable.
  vtkColorTransferFunction* ctf = vtkColorTransferFunction::SafeDownCast(stc);

  if (ctf != NULL && pwf == NULL)
  {
    vtkNew<vtkColorTransferFunctionItem> item;
    item->SetColorTransferFunction(ctf);

    this->Internals->TransferFunctionItem = item.GetPointer();

    if (stc_editable)
    {
      vtkNew<vtkColorTransferControlPointsItem> cpItem;
      cpItem->SetColorTransferFunction(ctf);
      cpItem->SetColorFill(true);
      cpItem->SetEndPointsXMovable(false);
      cpItem->SetEndPointsYMovable(false);
      cpItem->SetLabelFormat("%.3f");
      this->Internals->ControlPointsItem = cpItem.GetPointer();

      this->Internals->CurrentPointEditEventId =
        cpItem->AddObserver(vtkControlPointsItem::CurrentPointEditEvent, this,
          &pqTransferFunctionWidget::onCurrentPointEditEvent);
    }
  }
  else if (ctf == NULL && pwf != NULL)
  {
    vtkNew<vtkPiecewiseFunctionItem> item;
    item->SetPiecewiseFunction(pwf);

    this->Internals->TransferFunctionItem = item.GetPointer();

    if (pwf_editable)
    {
      vtkNew<vtkPiecewiseControlPointsItem> cpItem;
      cpItem->SetPiecewiseFunction(pwf);
      cpItem->SetEndPointsXMovable(false);
      cpItem->SetEndPointsYMovable(true);
      cpItem->SetLabelFormat("%.3f: %.3f");
      this->Internals->ControlPointsItem = cpItem.GetPointer();
    }
  }
  else if (ctf != NULL && pwf != NULL)
  {
    vtkNew<vtkCompositeTransferFunctionItem> item;
    item->SetOpacityFunction(pwf);
    item->SetColorTransferFunction(ctf);
    item->SetMaskAboveCurve(true);

    this->Internals->TransferFunctionItem = item.GetPointer();
    if (pwf_editable && stc_editable)
    {
      // NOTE: this hasn't been tested yet.
      vtkNew<vtkCompositeControlPointsItem> cpItem;
      cpItem->SetPointsFunction(vtkCompositeControlPointsItem::ColorAndOpacityPointsFunction);
      cpItem->SetOpacityFunction(pwf);
      cpItem->SetColorTransferFunction(ctf);
      cpItem->SetEndPointsXMovable(false);
      cpItem->SetEndPointsYMovable(true);
      cpItem->SetUseOpacityPointHandles(true);
      cpItem->SetLabelFormat("%.3f: %.3f");
      this->Internals->ControlPointsItem = cpItem.GetPointer();
    }
    else if (pwf_editable)
    {
      vtkNew<vtkCompositeControlPointsItem> cpItem;
      cpItem->SetPointsFunction(vtkCompositeControlPointsItem::OpacityPointsFunction);
      cpItem->SetOpacityFunction(pwf);
      cpItem->SetColorTransferFunction(ctf);
      cpItem->SetEndPointsXMovable(false);
      cpItem->SetEndPointsYMovable(true);
      cpItem->SetUseOpacityPointHandles(true);
      cpItem->SetLabelFormat("%.3f: %.3f");
      this->Internals->ControlPointsItem = cpItem.GetPointer();
    }
  }
  else
  {
    return;
  }

  this->Internals->ChartXY->AddPlot(this->Internals->TransferFunctionItem);

  if (this->Internals->ControlPointsItem)
  {
    this->Internals->ControlPointsItem->SetEndPointsRemovable(false);
    this->Internals->ControlPointsItem->SetShowLabels(true);
    this->Internals->ChartXY->AddPlot(this->Internals->ControlPointsItem);

    pqCoreUtilities::connect(this->Internals->ControlPointsItem,
      vtkControlPointsItem::CurrentPointChangedEvent, this, SLOT(onCurrentChangedEvent()));
    pqCoreUtilities::connect(this->Internals->ControlPointsItem, vtkCommand::EndEvent, this,
      SIGNAL(controlPointsModified()));
  }

  // If the transfer functions change, we need to re-render the view. This
  // ensures that.
  if (ctf)
  {
    this->Internals->VTKConnect->Connect(
      ctf, vtkCommand::ModifiedEvent, this, SIGNAL(ctfModified()));
    QObject::connect(this, &pqTransferFunctionWidget::ctfModified, [ctf, this]() {
      auto chartXY = this->Internals->ChartXY.GetPointer();
      chartXY->SetTFRange(vtkVector2d(ctf->GetRange()));
      this->render();
    });
    auto chartXY = this->Internals->ChartXY.GetPointer();
    chartXY->SetTFRange(vtkVector2d(ctf->GetRange()));
  }
  if (pwf)
  {
    this->Internals->VTKConnect->Connect(
      pwf, vtkCommand::ModifiedEvent, this, SIGNAL(pwfModified()));
    QObject::connect(this, &pqTransferFunctionWidget::pwfModified, [pwf, this]() {
      auto chartXY = this->Internals->ChartXY.GetPointer();
      chartXY->SetTFRange(vtkVector2d(pwf->GetRange()));
      this->render();
    });
    auto chartXY = this->Internals->ChartXY.GetPointer();
    chartXY->SetTFRange(vtkVector2d(pwf->GetRange()));
  }
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::onCurrentPointEditEvent()
{
  vtkColorTransferControlPointsItem* cpitem =
    vtkColorTransferControlPointsItem::SafeDownCast(this->Internals->ControlPointsItem);
  if (cpitem == NULL)
  {
    return;
  }

  vtkIdType currentIdx = cpitem->GetCurrentPoint();
  if (currentIdx < 0)
  {
    return;
  }

  vtkColorTransferFunction* ctf = cpitem->GetColorTransferFunction();
  assert(ctf != NULL);

  double xrgbms[6];
  ctf->GetNodeValue(currentIdx, xrgbms);
  QColor color = QColorDialog::getColor(QColor::fromRgbF(xrgbms[1], xrgbms[2], xrgbms[3]), this,
    "Select Color", QColorDialog::DontUseNativeDialog);
  if (color.isValid())
  {
    xrgbms[1] = color.redF();
    xrgbms[2] = color.greenF();
    xrgbms[3] = color.blueF();
    ctf->SetNodeValue(currentIdx, xrgbms);

    emit this->controlPointsModified();
  }
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::onCurrentChangedEvent()
{
  if (this->Internals->ControlPointsItem)
  {
    emit this->currentPointChanged(this->Internals->ControlPointsItem->GetCurrentPoint());
  }
}

//-----------------------------------------------------------------------------
vtkIdType pqTransferFunctionWidget::currentPoint() const
{
  if (this->Internals->ControlPointsItem)
  {
    return this->Internals->ControlPointsItem->GetCurrentPoint();
  }

  return -1;
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::setCurrentPoint(vtkIdType index)
{
  if (this->Internals->ControlPointsItem)
  {
    if (index < -1 || index >= this->Internals->ControlPointsItem->GetNumberOfPoints())
    {
      index = -1;
    }
    this->Internals->ControlPointsItem->SetCurrentPoint(index);
  }
}

//-----------------------------------------------------------------------------
vtkIdType pqTransferFunctionWidget::numberOfControlPoints() const
{
  return this->Internals->ControlPointsItem
    ? this->Internals->ControlPointsItem->GetNumberOfPoints()
    : 0;
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::SetLogScaleXAxis(bool logScale)
{
  this->Internals->ChartXY->GetAxis(vtkAxis::BOTTOM)->SetLogScale(logScale);
}

//-----------------------------------------------------------------------------
bool pqTransferFunctionWidget::GetLogScaleXAxis() const
{
  return this->Internals->ChartXY->GetAxis(vtkAxis::BOTTOM)->GetLogScale();
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::render()
{
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::setCurrentPointPosition(double xpos)
{
  vtkIdType currentPid = this->currentPoint();
  if (currentPid < 0)
  {
    return;
  }

  vtkIdType numPts = this->Internals->ControlPointsItem->GetNumberOfPoints();
  if (currentPid >= 0)
  {
    double start_point[4];
    this->Internals->ControlPointsItem->GetControlPoint(0, start_point);
    xpos = std::max(start_point[0], xpos);
  }
  if (currentPid <= (numPts - 1))
  {
    double end_point[4];
    this->Internals->ControlPointsItem->GetControlPoint(numPts - 1, end_point);
    xpos = std::min(end_point[0], xpos);
  }

  double point[4];
  this->Internals->ControlPointsItem->GetControlPoint(currentPid, point);
  if (point[0] != xpos)
  {
    point[0] = xpos;
    this->Internals->ControlPointsItem->SetControlPoint(currentPid, point);
  }
}
