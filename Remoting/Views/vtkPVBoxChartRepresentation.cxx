/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBoxChartRepresentation.h"

#include "vtkAxis.h"
#include "vtkChartBox.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotBox.h"
#include "vtkTable.h"

#include <map>
#include <string>

class vtkPVBoxChartRepresentation::vtkInternals
{
public:
  // Refer to vtkPVPlotMatrixRepresentation when time comes to update this
  // representation to respect the order in which the series are listed.
  std::map<std::string, bool> SeriesVisibilities;
  std::map<std::string, vtkColor3d> SeriesColors;
};

vtkStandardNewMacro(vtkPVBoxChartRepresentation);
//----------------------------------------------------------------------------
vtkPVBoxChartRepresentation::vtkPVBoxChartRepresentation()
  : LineThickness(2)
  , LineStyle(0)
  , Legend(true)
{
  this->Internals = new vtkInternals();
  this->Color[0] = this->Color[1] = this->Color[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkPVBoxChartRepresentation::~vtkPVBoxChartRepresentation()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVBoxChartRepresentation::AddToView(vtkView* view)
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
bool vtkPVBoxChartRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GetChart())
  {
    this->GetChart()->GetPlot(0)->SetInputData(nullptr);
    this->GetChart()->SetVisible(false);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
vtkChartBox* vtkPVBoxChartRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartBox::SafeDownCast(this->ContextView->GetContextItem());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartBox* chart = this->GetChart();
  if (chart && !visible)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    chart->SetVisible(false);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::SetSeriesVisibility(const char* series, bool visibility)
{
  assert(series != nullptr);
  this->Internals->SeriesVisibilities[series] = visibility;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::SetSeriesColor(
  const char* seriesname, double r, double g, double b)
{
  assert(seriesname != nullptr);
  this->Internals->SeriesColors[seriesname] = vtkColor3d(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::ClearSeriesColors()
{
  this->Internals->SeriesColors.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBoxChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartBox* chart = this->GetChart();
  vtkPlotBox* plot = vtkPlotBox::SafeDownCast(chart->GetPlot(0));
  assert(plot);

  vtkTable* plotInput = this->GetLocalOutput();
  chart->SetVisible(plotInput != nullptr && this->GetVisibility());
  chart->SetShowLegend(this->Legend);

  plot->GetPen()->SetWidth(this->LineThickness);
  plot->GetPen()->SetLineType(this->LineStyle);
  plot->GetPen()->SetColorF(this->Color[0], this->Color[1], this->Color[2]);
  plot->GetPen()->SetOpacityF(1.0);

  if (plotInput)
  {
    // only consider the first vtkTable.
    plot->SetInputData(plotInput);
    chart->SetColumnVisibilityAll(true);
    vtkIdType numCols = plotInput->GetNumberOfColumns();
    for (vtkIdType cc = 0; cc < numCols; cc++)
    {
      std::string name = plotInput->GetColumnName(cc);
      std::map<std::string, bool>::iterator iter = this->Internals->SeriesVisibilities.find(name);

      if (iter != this->Internals->SeriesVisibilities.end() && iter->second == true)
      {
        chart->SetColumnVisibility(name, true);
      }
      else
      {
        chart->SetColumnVisibility(name, false);
      }

      std::map<std::string, vtkColor3d>::iterator citer = this->Internals->SeriesColors.find(name);
      if (citer != this->Internals->SeriesColors.end())
      {
        plot->SetColumnColor(name, citer->second.GetData());
      }
    }
  }

  if (chart->GetYAxis())
  {
    chart->GetYAxis()->SetTitle("");
  }
}
