// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTransferFunction2DWidget.h"

#include "QVTKRenderWidget.h"
#include "pqCoreUtilities.h"
#include "pqTimer.h"
#include "vtkAxis.h"
#include "vtkChartHistogram2D.h"
#include "vtkColorTransferFunction.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkControlPointsItem.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkFloatArray.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVTransferFunction2D.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPointData.h"
#include "vtkTransferFunctionBoxItem.h"
#include "vtkTransferFunctionChartHistogram2D.h"

// Qt includes
#include <QMainWindow>
#include <QStatusBar>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
class pqTransferFunction2DWidget::pqInternals
{
  vtkNew<vtkGenericOpenGLRenderWindow> Window;

public:
  QPointer<QVTKRenderWidget> Widget;
  vtkNew<vtkTransferFunctionChartHistogram2D> Chart;
  vtkNew<vtkContextView> ContextView;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  vtkWeakPointer<vtkPVTransferFunction2D> TransferFunction;

  pqTimer Timer;

  pqInternals(pqTransferFunction2DWidget* editor)
    : Widget(new QVTKRenderWidget(editor))
  {
    this->Timer.setSingleShot(true);
    this->Timer.setInterval(0);

    this->Window->SetMultiSamples(8);

    this->Widget->setEnableHiDPI(true);
    this->Widget->setObjectName("1QVTKRenderWidget0");
    this->Widget->setRenderWindow(this->Window);
    this->ContextView->SetRenderWindow(this->Window);

    this->Widget->setParent(editor);
    QVBoxLayout* layout = new QVBoxLayout(editor);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(this->Widget);

    this->Chart->SetAutoSize(true);
    this->Chart->SetShowLegend(true);
    this->Chart->SetZoomWithMouseWheel(false);
    this->ContextView->GetScene()->AddItem(this->Chart);
    this->ContextView->SetInteractor(this->Widget->interactor());
    this->ContextView->GetRenderWindow()->SetLineSmoothing(true);

    this->Chart->SetActionToButton(vtkChart::PAN, -1);
    this->Chart->SetActionToButton(vtkChart::ZOOM, -1);

    for (int cc = 0; cc < 4; cc++)
    {
      this->Chart->GetAxis(cc)->SetVisible(false);
      this->Chart->GetAxis(cc)->SetBehavior(vtkAxis::FIXED);
    }
    this->Chart->SetRenderEmpty(true);
    this->Chart->SetAutoAxes(true);
    this->Chart->SetHiddenAxisBorder(8);
    auto axis = this->Chart->GetAxis(vtkAxis::BOTTOM);
    axis->SetTitle("Scalar Value");
    axis->SetRange(0, 255);

    axis = this->Chart->GetAxis(vtkAxis::LEFT);
    axis->SetTitle("Scalar Value");
    axis->SetRange(0, 255);
  }

  ~pqInternals() { this->cleanup(); }

  void cleanup()
  {
    this->VTKConnect->Disconnect();
    this->Chart->ClearPlots();
  }

  void initialize(vtkPVTransferFunction2D* tf2d)
  {
    this->TransferFunction = tf2d;
    this->Chart->SetTransferFunction2D(tf2d);
    // this->Chart->SetTransfer2DBoxesItem(ctf);
    //    if (this->Chart->IsInitialized())
    //    {
    //      this->Chart->AddNewBox();
    //    }
  }

  void setHistogram(vtkImageData* histogram)
  {
    this->Chart->SetInputData(histogram);

    if (!histogram)
    {
      return;
    }
    vtkDataArray* arr = histogram->GetPointData()->GetScalars();
    if (!arr)
    {
      return;
    }
    double range[2];
    arr->GetRange(range, 0);

    // A minimum of 1.0 is used in order to clip off histogram bins with a
    // single occurrence. This is also necessary to enable Log10 scale (required
    // to have min > 0)
    vtkNew<vtkColorTransferFunction> transferFunction;
    transferFunction->AddRGBSegment(range[0] + 1.0, 0.0, 0.0, 0.0, range[1], 1.0, 1.0, 1.0);

    transferFunction->SetScaleToLog10();
    transferFunction->Build();
    this->Chart->SetTransferFunction(transferFunction);
  }
};

//-----------------------------------------------------------------------------
pqTransferFunction2DWidget::pqTransferFunction2DWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqInternals(this))
{
  // whenever the rendering timer times out, we render the widget.
  QObject::connect(&this->Internals->Timer, &QTimer::timeout, [this]() {
    auto renWin = this->Internals->ContextView->GetRenderWindow();
    if (this->isVisible())
    {
      renWin->Render();
    }
  });
}

//-----------------------------------------------------------------------------
pqTransferFunction2DWidget::~pqTransferFunction2DWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
vtkImageData* pqTransferFunction2DWidget::histogram() const
{
  if (auto plot = vtkPlotHistogram2D::SafeDownCast(this->Internals->Chart->GetPlot(0)))
  {
    return vtkImageData::SafeDownCast(plot->GetInputImageData());
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqTransferFunction2DWidget::setHistogram(vtkImageData* histogram)
{
  this->Internals->setHistogram(histogram);
  this->render();
}

//-----------------------------------------------------------------------------
void pqTransferFunction2DWidget::initialize(vtkPVTransferFunction2D* tf2d)
{
  if (!tf2d)
  {
    return;
  }

  this->Internals->initialize(tf2d);

  pqCoreUtilities::connect(this->Internals->Widget->interactor(), vtkCommand::MouseMoveEvent, this,
    SLOT(showUsageStatus()));
  pqCoreUtilities::connect(this->Internals->Chart,
    vtkTransferFunctionChartHistogram2D::TransferFunctionModified, this,
    SIGNAL(transferFunctionModified()));
}

//-----------------------------------------------------------------------------
bool pqTransferFunction2DWidget::isInitialized()
{
  return this->Internals->Chart->IsInitialized();
}

//-----------------------------------------------------------------------------
void pqTransferFunction2DWidget::render()
{
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqTransferFunction2DWidget::showUsageStatus()
{
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  if (mainWindow)
  {
    mainWindow->statusBar()->showMessage(tr("Double click to add a box. "
                                            "Grab and drag to move the box."),
      2000);
  }
}

//-----------------------------------------------------------------------------
vtkChart* pqTransferFunction2DWidget::chart() const
{
  return this->Internals->Chart;
}

//-----------------------------------------------------------------------------
vtkPVTransferFunction2D* pqTransferFunction2DWidget::transferFunction() const
{
  return this->Internals->TransferFunction;
}
