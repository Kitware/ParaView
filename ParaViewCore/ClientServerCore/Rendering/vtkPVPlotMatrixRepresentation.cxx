/*=========================================================================

   Program: ParaView
   Module:    pqPlotMatrixDisplayPanel.h

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

=========================================================================*/
#include "vtkPVPlotMatrixRepresentation.h"

#include "vtkAnnotationLink.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPlotPoints.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkPVPlotMatrixRepresentation);

class vtkPVPlotMatrixRepresentation::vtkInternals
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

//----------------------------------------------------------------------------
vtkPVPlotMatrixRepresentation::vtkPVPlotMatrixRepresentation()
{
  this->Internals = new vtkInternals();

  // default colors are black (0, 0, 0)
  for (int i = 0; i < 3; i++)
  {
    this->ScatterPlotColor[i] = 0;
    this->ActivePlotColor[i] = 0;
    this->HistogramColor[i] = 0;
  }
  this->ScatterPlotColor[3] = 255;
  this->ActivePlotColor[3] = 255;
  this->HistogramColor[3] = 255;

  this->ScatterPlotMarkerStyle = vtkPlotPoints::CIRCLE;
  this->ActivePlotMarkerStyle = vtkPlotPoints::CIRCLE;
  this->ScatterPlotMarkerSize = 5.0;
  this->ActivePlotMarkerSize = 8.0;
}

//----------------------------------------------------------------------------
vtkPVPlotMatrixRepresentation::~vtkPVPlotMatrixRepresentation()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
bool vtkPVPlotMatrixRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
  {
    return false;
  }

  if (vtkScatterPlotMatrix* plotMatrix = this->GetPlotMatrix())
  {
    plotMatrix->SetVisible(this->GetVisibility());
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVPlotMatrixRepresentation::RemoveFromView(vtkView* view)
{
  if (vtkScatterPlotMatrix* plotMatrix = this->GetPlotMatrix())
  {
    plotMatrix->SetInput(0);
    plotMatrix->SetVisible(false);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkScatterPlotMatrix* plotMatrix = this->GetPlotMatrix();

  // set chart properties
  plotMatrix->SetPlotColor(vtkScatterPlotMatrix::SCATTERPLOT, this->ScatterPlotColor);
  plotMatrix->SetPlotColor(vtkScatterPlotMatrix::HISTOGRAM, this->HistogramColor);
  plotMatrix->SetPlotColor(vtkScatterPlotMatrix::ACTIVEPLOT, this->ActivePlotColor);
  plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::SCATTERPLOT, this->ScatterPlotMarkerStyle);
  plotMatrix->SetPlotMarkerStyle(vtkScatterPlotMatrix::ACTIVEPLOT, this->ActivePlotMarkerStyle);
  plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::SCATTERPLOT, this->ScatterPlotMarkerSize);
  plotMatrix->SetPlotMarkerSize(vtkScatterPlotMatrix::ACTIVEPLOT, this->ActivePlotMarkerSize);

  // vtkPVPlotMatrixRepresentation doesn't support multiblock of tables, so we
  // only consider the first vtkTable.
  vtkTable* table = this->GetLocalOutput();
  plotMatrix->SetVisible(table != NULL && this->GetVisibility());
  if (plotMatrix && table)
  {
    plotMatrix->SetInput(table);

    // Set column visibilities
    vtkIdType numCols = table->GetNumberOfColumns();
    for (vtkIdType cc = 0; cc < numCols; cc++)
    {
      const char* name = table->GetColumnName(cc);
      plotMatrix->SetColumnVisibility(name, this->Internals->GetSeriesVisibility(name));
    }

    vtkSmartPointer<vtkStringArray> orderedVisibleColumns =
      this->Internals->GetOrderedVisibleColumnNames(table);

    // this is essential since SetVisibleColumns doesn't seem to resize the
    // matrix to the new size (is that a bug?).
    plotMatrix->SetSize(vtkVector2i(
      orderedVisibleColumns->GetNumberOfTuples(), orderedVisibleColumns->GetNumberOfTuples()));

    // Set column order
    plotMatrix->SetVisibleColumns(orderedVisibleColumns.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  vtkScatterPlotMatrix* plotMatrix = this->GetPlotMatrix();
  if (plotMatrix && !visible)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    plotMatrix->SetVisible(false);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetSeriesVisibility(const char* series, bool visibility)
{
  assert(series != NULL);
  this->Internals->SeriesVisibilities.push_back(std::pair<std::string, bool>(series, visibility));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetColor(double r, double g, double b)
{
  this->ScatterPlotColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
    static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255));

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotColor(double r, double g, double b)
{
  this->ActivePlotColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
    static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetHistogramColor(double r, double g, double b)
{
  this->HistogramColor = vtkColor4ub(static_cast<unsigned char>(r * 255),
    static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerStyle(int style)
{
  this->ScatterPlotMarkerStyle = style;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerStyle(int style)
{
  this->ActivePlotMarkerStyle = style;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetMarkerSize(double size)
{
  this->ScatterPlotMarkerSize = size;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::SetActivePlotMarkerSize(double size)
{
  this->ActivePlotMarkerSize = size;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPlotMatrixRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkScatterPlotMatrix* vtkPVPlotMatrixRepresentation::GetPlotMatrix() const
{
  if (this->ContextView)
  {
    return vtkScatterPlotMatrix::SafeDownCast(this->ContextView->GetContextItem());
  }

  return 0;
}
