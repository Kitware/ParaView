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
#include "vtkPlotHistogram2D.h"
#include "vtkPointData.h"
#include "vtkTransfer2DBoxItem.h"
#include "vtkXMLImageDataWriter.h"

// Qt includes
#include <QVBoxLayout>

namespace paraviewTransfer2D
{
class vtkTransferFunctionChartHistogram2D : public vtkChartHistogram2D
{
public:
  static vtkTransferFunctionChartHistogram2D* New();
  vtkTypeMacro(vtkTransferFunctionChartHistogram2D, vtkChartHistogram2D);

  //-----------------------------------------------------------------------------
  bool IsInitialized()
  {
    auto plot = vtkPlotHistogram2D::SafeDownCast(this->GetPlot(0));
    if (!plot)
    {
      return false;
    }
    if (plot->GetInputImageData())
    {
      return true;
    }
    return false;
  }

  //-----------------------------------------------------------------------------
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override
  {
    if (this->IsInitialized())
    {
      this->AddNewBox();
    }
    return Superclass::MouseDoubleClickEvent(mouse);
  }

  //-----------------------------------------------------------------------------
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override
  {
    if (this->IsInitialized())
    {
      if (mouse.GetButton() == vtkContextMouseEvent::RIGHT_BUTTON)
      {
        vtkSmartPointer<vtkImageData> im = this->GetTransfer2D();
        if (im)
        {
          vtkNew<vtkXMLImageDataWriter> w;
          w->SetInputData(im);
          w->SetFileName("Transfer2D.vti");
          w->Write();
        }
      }
    }
    return Superclass::MouseButtonPressEvent(mouse);
  }

  //-----------------------------------------------------------------------------
  vtkSmartPointer<vtkTransfer2DBoxItem> AddNewBox()
  {
    double xRange[2];
    auto bottomAxis = this->GetAxis(vtkAxis::BOTTOM);
    bottomAxis->GetRange(xRange);

    double yRange[2];
    auto leftAxis = this->GetAxis(vtkAxis::LEFT);
    leftAxis->GetRange(yRange);

    vtkNew<vtkTransfer2DBoxItem> boxItem;
    // Set bounds in the box item so that it can only move within the
    // histogram's range.
    boxItem->SetValidBounds(xRange[0], xRange[1], yRange[0], yRange[1]);
    const double width = (xRange[1] - xRange[0]) / 3.0;
    const double height = (yRange[1] - yRange[0]) / 3.0;
    boxItem->SetBox(xRange[0] + width, yRange[0] + height, width, height);
    // boxItem->AddObserver(vtkCommand::SelectionChangedEvent, Callback.GetPointer());
    this->AddPlot(boxItem);
    return boxItem;
  }

  //-----------------------------------------------------------------------------
  void SetInputData(vtkImageData* data, vtkIdType z = 0) override
  {
    if (data)
    {
      int bins[3];
      double origin[3], spacing[3];
      data->GetOrigin(origin);
      data->GetDimensions(bins);
      data->GetSpacing(spacing);

      // Compute image bounds
      const double xMin = origin[0];
      const double xMax = bins[0] * spacing[0];
      const double yMin = origin[1];
      const double yMax = bins[1] * spacing[1];

      auto axis = GetAxis(vtkAxis::BOTTOM);
      axis->SetUnscaledRange(xMin, xMax);
      axis = GetAxis(vtkAxis::LEFT);
      axis->SetUnscaledRange(yMin, yMax);
      this->RecalculatePlotTransforms();

      UpdateItemsBounds(xMin, xMax, yMin, yMax);
    }
    vtkChartHistogram2D::SetInputData(data, z);
  }

  //-----------------------------------------------------------------------------
  void UpdateItemsBounds(const double xMin, const double xMax, const double yMin, const double yMax)
  {
    // Set the new bounds to its current box items (plots).
    const vtkIdType numPlots = GetNumberOfPlots();
    for (vtkIdType i = 0; i < numPlots; i++)
    {
      auto boxItem = vtkControlPointsItem::SafeDownCast(GetPlot(i));
      if (!boxItem)
      {
        continue;
      }

      boxItem->SetValidBounds(xMin, xMax, yMin, yMax);
    }
  }

  //-----------------------------------------------------------------------------
  vtkSmartPointer<vtkImageData> GetTransfer2D()
  {
    if (!this->IsInitialized())
    {
      return nullptr;
    }
    const vtkIdType numPlots = this->GetNumberOfPlots();
    if (numPlots < 2)
    {
      // the first plot will be the histogram plot
      // i.e. no transfer2D boxes
      return nullptr;
    }

    vtkSmartPointer<vtkImageData> histogram =
      vtkPlotHistogram2D::SafeDownCast(this->GetPlot(0))->GetInputImageData();
    if (!histogram)
    {
      return nullptr;
    }

    double spacing[3];
    histogram->GetSpacing(spacing);
    double origin[3];
    histogram->GetOrigin(origin);
    int dims[3];
    histogram->GetDimensions(dims);

    vtkNew<vtkImageData> im;
    // im->SetOrigin(origin);
    // im->SetSpacing(spacing);
    im->SetDimensions(dims);
    im->AllocateScalars(VTK_FLOAT, 4);
    auto arr = vtkFloatArray::SafeDownCast(im->GetPointData()->GetScalars());
    auto arrRange = vtk::DataArrayValueRange(arr);
    std::fill(arrRange.begin(), arrRange.end(), 0.0);

    for (vtkIdType i = 0; i < numPlots; ++i)
    {
      auto boxItem = vtkTransfer2DBoxItem::SafeDownCast(this->GetPlot(i));
      if (!boxItem)
      {
        continue;
      }
      const vtkRectd box = boxItem->GetBox();
      vtkIdType width = static_cast<vtkIdType>(box.GetWidth() / spacing[0]);
      vtkIdType height = static_cast<vtkIdType>(box.GetHeight() / spacing[1]);
      if (width <= 0 || height <= 0)
      {
        continue;
      }
      vtkIdType x0 = static_cast<vtkIdType>((box.GetX() - origin[0]) / spacing[0]);
      vtkIdType y0 = static_cast<vtkIdType>((box.GetY() - origin[1]) / spacing[1]);
      vtkSmartPointer<vtkImageData> boxTexture = boxItem->GetTexture();
      int boxDims[3];
      boxTexture->GetDimensions(boxDims);
      double boxSpacing[3] = { 1, 1, 1 };
      boxSpacing[0] = box.GetWidth() / boxDims[0];
      boxSpacing[1] = box.GetHeight() / boxDims[1];
      for (vtkIdType ii = x0; ii < x0 + width; ++ii)
      {
        for (vtkIdType jj = y0; jj < y0 + height; ++jj)
        {
          int boxCoord[3] = { 0, 0, 0 };
          boxCoord[0] = (ii - x0) * spacing[0] / boxSpacing[0];
          boxCoord[1] = (jj - y0) * spacing[1] / boxSpacing[1];
          unsigned char* ptr = static_cast<unsigned char*>(boxTexture->GetScalarPointer(boxCoord));
          float fptr[4];
          for (int tp = 0; tp < 4; ++tp)
          {
            fptr[tp] = ptr[tp] / 255.0;
          }
          // composite this color with the current color
          float* c = static_cast<float*>(im->GetScalarPointer(ii, jj, 0));
          float opacity = c[3] + fptr[3] * (1 - c[3]);
          opacity = opacity > 1.0 ? 1.0 : opacity;
          for (int tp = 0; tp < 3; ++tp)
          {
            c[tp] = (fptr[tp] * fptr[3] + c[tp] * c[3] * (1 - fptr[tp])) / opacity;
            c[tp] = c[tp] > 1.0 ? 1.0 : c[tp];
          }
          c[3] = opacity;
        }
      }
    }

    return im;
  }

protected:
  vtkTransferFunctionChartHistogram2D() {}
  ~vtkTransferFunctionChartHistogram2D() override = default;

  // Member variables;

private:
  vtkTransferFunctionChartHistogram2D(const vtkTransferFunctionChartHistogram2D&);
  void operator=(const vtkTransferFunctionChartHistogram2D&);
};
vtkStandardNewMacro(vtkTransferFunctionChartHistogram2D);
} // end of namespace paraviewTransfer2D

//-----------------------------------------------------------------------------
class pqTransferFunction2DWidget::pqInternals
{
  vtkNew<vtkGenericOpenGLRenderWindow> Window;

public:
  QPointer<QVTKRenderWidget> Widget;
  vtkNew<paraviewTransfer2D::vtkTransferFunctionChartHistogram2D> Chart;
  vtkNew<vtkContextView> ContextView;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

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
    layout->setMargin(0);
    layout->addWidget(this->Widget);

    this->Chart->SetAutoSize(true);
    this->Chart->SetShowLegend(true);
    this->Chart->SetZoomWithMouseWheel(false);
    this->ContextView->GetScene()->AddItem(this->Chart);
    this->ContextView->SetInteractor(this->Widget->interactor());
    this->ContextView->GetRenderWindow()->SetLineSmoothing(true);

    this->Chart->SetActionToButton(vtkChart::PAN, -1);
    this->Chart->SetActionToButton(vtkChart::ZOOM, -1);

    this->Chart->SetRenderEmpty(true);
    this->Chart->SetAutoAxes(false);
    this->Chart->SetHiddenAxisBorder(8);
    auto axis = this->Chart->GetAxis(vtkAxis::BOTTOM);
    axis->SetTitle("Scalar Value");
    axis->SetBehavior(vtkAxis::FIXED);
    axis->SetRange(0, 255);

    axis = this->Chart->GetAxis(vtkAxis::LEFT);
    axis->SetTitle("Scalar Value");
    axis->SetBehavior(vtkAxis::FIXED);
    axis->SetRange(0, 255);

    for (int cc = 0; cc < 4; cc++)
    {
      this->Chart->GetAxis(cc)->SetVisible(false);
      this->Chart->GetAxis(cc)->SetBehavior(vtkAxis::FIXED);
    }
  }

  ~pqInternals() { this->cleanup(); }

  void cleanup()
  {
    this->VTKConnect->Disconnect();
    this->Chart->ClearPlots();
  }

  void initialize()
  {
    if (this->Chart->IsInitialized())
    {
      return;
    }

    this->Chart->AddNewBox();
  }

  void setHistogram(vtkImageData* histogram)
  {
    this->Chart->SetInputData(histogram);

    if (!histogram)
    {
      this->ContextView->Render();
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

    this->ContextView->Render();
  }
};

//-----------------------------------------------------------------------------
pqTransferFunction2DWidget::pqTransferFunction2DWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqInternals(this))
{
  // whenever the rendering timer times out, we render the widget.
  /*QObject::connect(&this->Internals->Timer, &QTimer::timeout, [this]() {
    auto renWin = this->Internals->ContextView->GetRenderWindow();
    if (this->isVisible())
    {
      renWin->Render();
    }
  });*/
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
}

//-----------------------------------------------------------------------------
void pqTransferFunction2DWidget::initialize()
{
  if (!this->histogram())
  {
    return;
  }

  this->Internals->initialize();
}
