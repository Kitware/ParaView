// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVImageChartRepresentation.h"

#include "vtkChartHistogram2D.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPlotHistogram2D.h"
#include "vtkScalarsToColors.h"

vtkStandardNewMacro(vtkPVImageChartRepresentation);
vtkCxxSetSmartPointerMacro(vtkPVImageChartRepresentation, LookupTable, vtkScalarsToColors);

//----------------------------------------------------------------------------
vtkPVImageChartRepresentation::~vtkPVImageChartRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPVImageChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->LookupTable->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
bool vtkPVImageChartRepresentation::RemoveFromView(vtkView* view)
{
  vtkChartHistogram2D* chart = this->GetChart();
  if (chart)
  {
    chart->RemovePlot(0);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
vtkChartHistogram2D* vtkPVImageChartRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartHistogram2D::SafeDownCast(this->ContextView->GetContextItem());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVImageChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartHistogram2D* chart = this->GetChart();
  if (chart)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    vtkPlot* plot = chart->GetPlot(0);
    if (plot)
    {
      plot->SetVisible(visible);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVImageChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartHistogram2D* chart = this->GetChart();
  assert(chart != nullptr); // we are assured this is always so.

  // Create the plot if not existing yet
  vtkPlotHistogram2D* gridPlot = vtkPlotHistogram2D::SafeDownCast(chart->GetPlot(0));
  if (!gridPlot)
  {
    vtkNew<vtkPlotHistogram2D> newGridPlot;
    chart->AddPlot(newGridPlot);
    gridPlot = newGridPlot;
  }

  vtkSmartPointer<vtkImageData> localGrid;
  if (this->LocalOutput)
  {
    for (unsigned int idx = 0; idx < this->LocalOutput->GetNumberOfBlocks(); ++idx)
    {
      localGrid = vtkImageData::SafeDownCast(this->LocalOutput->GetBlock(idx));
      if (localGrid)
      {
        chart->SetInputData(localGrid);
        break;
      }
    }
  }

  gridPlot->SetVisible(localGrid != nullptr);

  if (localGrid)
  {
    vtkInformation* info = this->GetInputArrayInformation(0);
    std::string arrayName =
      info->Has(vtkDataObject::FIELD_NAME()) ? info->Get(vtkDataObject::FIELD_NAME()) : "";

    gridPlot->SetInputData(localGrid);
    gridPlot->SetArrayName(arrayName);
    gridPlot->SetTransferFunction(this->LookupTable);
    gridPlot->SetVisible(true);

    chart->SetTransferFunction(this->LookupTable);
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkPVImageChartRepresentation::ReduceDataToRoot(vtkDataObject* data)
{
  // We want to keep a vtkImageData, instead of a vtkTable
  return data;
}
