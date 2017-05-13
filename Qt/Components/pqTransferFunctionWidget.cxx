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

#include "pqCoreUtilities.h"
#include "pqQVTKWidgetBase.h"
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

#include <QColorDialog>
#include <QPointer>
#include <QVBoxLayout>
#include <algorithm>

#if QT_VERSION >= 0x050000
#include <QSurfaceFormat>
#endif

//-----------------------------------------------------------------------------
// We extend vtkChartXY to add logic to reset view bounds automatically. This
// ensures that when LUT changes, we are always showing the complete LUT.
class vtkTransferFunctionChartXY : public vtkChartXY
{
  double XRange[2];
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
  vtkWeakPointer<vtkControlPointsItem> ControlPointsItem;

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update() VTK_OVERRIDE
  {
    if (this->ControlPointsItem)
    {
      // Reset bounds if the control points' bounds have changed.
      double bounds[4];
      this->ControlPointsItem->GetBounds(bounds);
      this->SetVisible(true);
      if (bounds[0] <= bounds[1] && (bounds[0] != this->XRange[0] || bounds[1] != this->XRange[1]))
      {
        this->XRange[0] = bounds[0];
        this->XRange[1] = bounds[1];
        this->DataValid = this->IsDataRangeValid(this->XRange);
        this->RecalculateBounds();
      }
    }
    this->Superclass::Update();
  }

  virtual bool PaintChildren(vtkContext2D* painter) VTK_OVERRIDE
  {
    if (this->DataValid)
    {
      return this->Superclass::PaintChildren(painter);
    }
    painter->DrawString(5, 5, "Data range too small to render.");
    return true;
  }

  virtual bool MouseEnterEvent(const vtkContextMouseEvent& mouse) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseEnterEvent(mouse) : false);
  }
  virtual bool MouseMoveEvent(const vtkContextMouseEvent& mouse) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseMoveEvent(mouse) : false);
  }
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseLeaveEvent(mouse) : false);
  }
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseButtonPressEvent(mouse) : false);
  }
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseButtonReleaseEvent(mouse) : false);
  }
  virtual bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::MouseWheelEvent(mouse, delta) : false);
  }
  virtual bool KeyPressEvent(const vtkContextKeyEvent& key) VTK_OVERRIDE
  {
    return (this->DataValid ? this->Superclass::KeyPressEvent(key) : false);
  }

protected:
  vtkTransferFunctionChartXY()
  {
    this->XRange[0] = this->XRange[1] = 0.0;
    this->DataValid = false;
    this->ZoomWithMouseWheelOff();
  }
  virtual ~vtkTransferFunctionChartXY() {}

private:
  vtkTransferFunctionChartXY(const vtkTransferFunctionChartXY&);
  void operator=(const vtkTransferFunctionChartXY&);
};

vtkStandardNewMacro(vtkTransferFunctionChartXY);

//-----------------------------------------------------------------------------

class pqTransferFunctionWidget::pqInternals
{
  vtkNew<pqQVTKWidgetBaseRenderWindowType> Window;

public:
  QPointer<pqQVTKWidgetBase> Widget;
  vtkNew<vtkTransferFunctionChartXY> ChartXY;
  vtkNew<vtkContextView> ContextView;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

  pqTimer Timer;

  vtkSmartPointer<vtkScalarsToColorsItem> TransferFunctionItem;
  vtkSmartPointer<vtkControlPointsItem> ControlPointsItem;
  unsigned long CurrentPointEditEventId;

  pqInternals(pqTransferFunctionWidget* editor)
    : Widget(new pqQVTKWidgetBase(editor))
    , CurrentPointEditEventId(0)
  {
    this->Timer.setSingleShot(true);
    this->Timer.setInterval(0);

#if QT_VERSION >= 0x050000
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setSamples(8);
    this->Widget->setFormat(fmt);
    this->Widget->setEnableHiDPI(true);
#endif

    this->Widget->setObjectName("1QVTKWidget0");
    this->Widget->SetRenderWindow(this->Window.Get());
    this->ContextView->SetRenderWindow(this->Window.Get());

    this->ChartXY->SetAutoSize(true);
    this->ChartXY->SetShowLegend(false);
    this->ChartXY->SetForceAxesToBounds(true);
    this->ContextView->GetScene()->AddItem(this->ChartXY.GetPointer());
    this->ContextView->SetInteractor(this->Widget->GetInteractor());
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
      this->ChartXY->GetAxis(cc)->SetBehavior(vtkAxis::AUTO);
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
  QObject::connect(&this->Internals->Timer, SIGNAL(timeout()), this, SLOT(renderInternal()));
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
    this->Internals->ChartXY->ControlPointsItem = this->Internals->ControlPointsItem;
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
    this->Internals->VTKConnect->Connect(ctf, vtkCommand::ModifiedEvent, this, SLOT(render()));
  }
  if (pwf)
  {
    this->Internals->VTKConnect->Connect(pwf, vtkCommand::ModifiedEvent, this, SLOT(render()));
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
  Q_ASSERT(ctf != NULL);

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
void pqTransferFunctionWidget::render()
{
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqTransferFunctionWidget::renderInternal()
{
  if (this->isVisible() && this->Internals->ContextView->GetRenderWindow()->IsDrawable())
  {
    this->Internals->ContextView->GetRenderWindow()->Render();
  }
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
