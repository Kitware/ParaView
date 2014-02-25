/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBagChartRepresentation.h"

#include "vtkBrush.h"
#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotBag.h"
#include "vtkPVContextView.h"
#include "vtkTable.h"

#include <string>

vtkStandardNewMacro(vtkPVBagChartRepresentation);
//----------------------------------------------------------------------------
vtkPVBagChartRepresentation::vtkPVBagChartRepresentation() :
  LineThickness(1),
  LineStyle(0),
  Opacity(1.),
  PointSize(5),
  XAxisSeriesName(NULL),
  YAxisSeriesName(NULL),
  DensitySeriesName(NULL),
  UseIndexForXAxis(true)
{
  this->BagColor[0] = 1.0;
  this->BagColor[1] = this->BagColor[2] = 0.0;
  this->LineColor[0] = this->LineColor[1] = this->LineColor[2] = 0.0;
  this->PointColor[0] = this->PointColor[1] = this->PointColor[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkPVBagChartRepresentation::~vtkPVBagChartRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVBagChartRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  if (this->GetChart())
    {
    this->GetChart()->SetVisible(this->GetVisibility());
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVBagChartRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->SetInputData(0);
    this->GetChart()->SetVisible(false);
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartXY* chart = this->GetChart();
  if (chart && !visible)
    {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    chart->SetVisible(false);
    }

  this->Modified();
}

//----------------------------------------------------------------------------
vtkChartXY* vtkPVBagChartRepresentation::GetChart()
{
  if (this->ContextView)
    {
    return vtkChartXY::SafeDownCast(
      this->ContextView->GetContextItem());
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartXY* chart = this->GetChart();
  vtkPlotBag* plot = 0;
  for (int i = 0; i < chart->GetNumberOfPlots() && !plot; i++)
    {
    // Search for the first bag plot of the chart. For now
    // the chart only manage one bag plot
    plot = vtkPlotBag::SafeDownCast(chart->GetPlot(i));
    }
  if (!plot)
    {
    // Create and add a bag plot to the chart
    plot = vtkPlotBag::SafeDownCast(chart->AddPlot(vtkChart::BAG));
    }

  vtkTable* plotInput = this->GetLocalOutput();
  chart->SetVisible(plotInput != NULL && this->GetVisibility());

  // Set line (bag boundaries) pen properties
  plot->GetLinePen()->SetWidth(this->LineThickness);
  plot->GetLinePen()->SetLineType(this->LineStyle);
  plot->GetLinePen()->SetColorF(this->LineColor);
  plot->GetLinePen()->SetOpacityF(this->Opacity);

  // Set bag (polygons) brush properties
  plot->GetBrush()->SetColorF(this->BagColor);
  plot->GetBrush()->SetOpacityF(this->Opacity);

  // Set point pen properties
  plot->GetPen()->SetWidth(this->PointSize);
  plot->GetPen()->SetLineType(this->LineStyle);
  plot->GetPen()->SetColorF(this->PointColor);
  plot->GetPen()->SetOpacityF(1.0);

  if (plotInput)
    {
    // We only consider the first vtkTable.
    if (this->YAxisSeriesName && this->DensitySeriesName)
      {
      plot->SetUseIndexForXSeries(this->UseIndexForXAxis);
      if (this->UseIndexForXAxis)
        {
        plot->SetInputData(plotInput,
          this->YAxisSeriesName, this->DensitySeriesName);
        }
      else
        {
        plot->SetInputData(plotInput, this->XAxisSeriesName,
          this->YAxisSeriesName, this->DensitySeriesName);
        }
      }
    }
}
