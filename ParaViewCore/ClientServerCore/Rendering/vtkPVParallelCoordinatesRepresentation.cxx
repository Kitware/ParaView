/*=========================================================================

  Program:   ParaView
  Module:    vtkPVParallelCoordinatesRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVParallelCoordinatesRepresentation.h"

#include "vtkCSVExporter.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkStdString.h"
#include "vtkTable.h"

#include <map>
#include <string>

class vtkPVParallelCoordinatesRepresentation::vtkInternals
{
public:
  // Refer to vtkPVPlotMatrixRepresentation when time comes to update this
  // representation to respect the order in which the series are listed.
  std::map<std::string, bool> SeriesVisibilities;
};

vtkStandardNewMacro(vtkPVParallelCoordinatesRepresentation);
//----------------------------------------------------------------------------
vtkPVParallelCoordinatesRepresentation::vtkPVParallelCoordinatesRepresentation()
  : LineThickness(1)
  , LineStyle(0)
  , Opacity(0.1)
{
  this->Internals = new vtkInternals();
  this->Color[0] = this->Color[1] = this->Color[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkPVParallelCoordinatesRepresentation::~vtkPVParallelCoordinatesRepresentation()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVParallelCoordinatesRepresentation::AddToView(vtkView* view)
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
bool vtkPVParallelCoordinatesRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GetChart())
  {
    this->GetChart()->GetPlot(0)->SetInputData(0);
    this->GetChart()->SetVisible(false);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
vtkChartParallelCoordinates* vtkPVParallelCoordinatesRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartParallelCoordinates::SafeDownCast(this->ContextView->GetContextItem());
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartParallelCoordinates* chart = this->GetChart();
  if (chart && !visible)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    chart->SetVisible(false);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetSeriesVisibility(
  const char* series, bool visibility)
{
  assert(series != NULL);
  this->Internals->SeriesVisibilities[series] = visibility;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartParallelCoordinates* chart = this->GetChart();
  vtkTable* plotInput = this->GetLocalOutput();
  chart->SetVisible(plotInput != NULL && this->GetVisibility());
  chart->GetPlot(0)->GetPen()->SetWidth(this->LineThickness);
  chart->GetPlot(0)->GetPen()->SetLineType(this->LineStyle);
  chart->GetPlot(0)->GetPen()->SetColorF(this->Color[0], this->Color[1], this->Color[2]);
  chart->GetPlot(0)->GetPen()->SetOpacityF(this->Opacity);

  if (plotInput)
  {
    // only consider the first vtkTable.
    chart->GetPlot(0)->SetInputData(plotInput);
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
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPVParallelCoordinatesRepresentation::Export(vtkCSVExporter* exporter)
{
  vtkChartParallelCoordinates* chart = this->GetChart();
  vtkTable* plotInput = this->GetLocalOutput();
  if (!plotInput || this->GetVisibility() == false)
  {
    return false;
  }
  const vtkIdType numCols = plotInput->GetNumberOfColumns();
  for (vtkIdType cc = 0; cc < numCols; cc++)
  {
    std::string name = plotInput->GetColumnName(cc);
    if (chart->GetColumnVisibility(name))
    {
      exporter->AddColumn(plotInput->GetColumnByName(name.c_str()), name.c_str());
    }
  }
  return true;
}
