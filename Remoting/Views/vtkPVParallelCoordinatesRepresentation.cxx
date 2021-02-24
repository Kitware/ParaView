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
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <string>
#include <vector>

class vtkPVParallelCoordinatesRepresentation::vtkInternals
{
public:
  std::vector<std::pair<std::string, bool> > SeriesVisibilities;

  vtkSmartPointer<vtkStringArray> GetOrderedVisibleColumnNames(vtkTable* table)
  {
    vtkSmartPointer<vtkStringArray> result = vtkSmartPointer<vtkStringArray>::New();

    std::set<std::string> columnNames;

    vtkIdType numCols = table->GetNumberOfColumns();
    for (vtkIdType cc = 0; cc < numCols; cc++)
    {
      columnNames.insert(table->GetColumnName(cc));
    }

    for (size_t cc = 0; cc < this->SeriesVisibilities.size(); ++cc)
    {
      if (this->SeriesVisibilities[cc].second == true &&
        columnNames.find(this->SeriesVisibilities[cc].first) != columnNames.end())
      {
        result->InsertNextValue(this->SeriesVisibilities[cc].first.c_str());
      }
    }

    return result;
  }

  bool GetSeriesVisibility(const std::string& name)
  {
    for (size_t cc = 0; cc < this->SeriesVisibilities.size(); ++cc)
    {
      if (this->SeriesVisibilities[cc].first == name)
      {
        return this->SeriesVisibilities[cc].second;
      }
    }
    return false;
  }
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
  this->Internals = nullptr;
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
    this->GetChart()->GetPlot(0)->SetInputData(nullptr);
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

  return nullptr;
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
  assert(series != nullptr);
  this->Internals->SeriesVisibilities.push_back(std::pair<std::string, bool>(series, visibility));
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
  chart->SetVisible(plotInput != nullptr && this->GetVisibility());
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
      chart->SetColumnVisibility(name, this->Internals->GetSeriesVisibility(name));
    }

    vtkSmartPointer<vtkStringArray> orderedVisibleColumns =
      this->Internals->GetOrderedVisibleColumnNames(plotInput);

    // Set column order
    chart->SetVisibleColumns(orderedVisibleColumns.GetPointer());
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
