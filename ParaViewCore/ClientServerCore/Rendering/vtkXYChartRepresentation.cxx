/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXYChartRepresentation.h"
#include "vtkXYChartRepresentationInternals.h"

#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkScalarsToColors.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>

//-----------------------------------------------------------------------------
#define vtkCxxSetChartTypeMacro(_name, _value)                                                     \
  void vtkXYChartRepresentation::SetChartTypeTo##_name() { this->SetChartType(_value); }
vtkCxxSetChartTypeMacro(Line, vtkChart::LINE);
vtkCxxSetChartTypeMacro(Points, vtkChart::POINTS);
vtkCxxSetChartTypeMacro(Bar, vtkChart::BAR);
vtkCxxSetChartTypeMacro(Stacked, vtkChart::STACKED);
vtkCxxSetChartTypeMacro(Bag, vtkChart::BAG);
vtkCxxSetChartTypeMacro(FunctionalBag, vtkChart::FUNCTIONALBAG);
vtkCxxSetChartTypeMacro(Area, vtkChart::AREA);
vtkStandardNewMacro(vtkXYChartRepresentation);
//----------------------------------------------------------------------------
vtkXYChartRepresentation::vtkXYChartRepresentation()
  : Internals(new vtkXYChartRepresentation::vtkInternals())
  , ChartType(vtkChart::LINE)
  , XAxisSeriesName(NULL)
  , UseIndexForXAxis(true)
  , PlotDataHasChanged(false)
  , SeriesLabelPrefix(NULL)
{
  this->SelectionColor[0] = 1.;
  this->SelectionColor[1] = 0.;
  this->SelectionColor[2] = 1.;
}

//----------------------------------------------------------------------------
vtkXYChartRepresentation::~vtkXYChartRepresentation()
{
  if (this->GetChart())
  {
    this->Internals->RemoveAllPlots(this->GetChart());
  }
  delete this->Internals;
  this->Internals = NULL;
  this->SetXAxisSeriesName(NULL);
  this->SetSeriesLabelPrefix(NULL);
}

//----------------------------------------------------------------------------
bool vtkXYChartRepresentation::RemoveFromView(vtkView* view)
{
  if ((this->ContextView.GetPointer() == view) && (this->GetChart() != NULL))
  {
    this->Internals->RemoveAllPlots(this->GetChart());
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  if (this->GetChart() && !visible)
  {
    // Hide all plots.
    this->Internals->HideAllPlots();
  }
}

//----------------------------------------------------------------------------
vtkChartXY* vtkXYChartRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartXY::SafeDownCast(this->ContextView->GetContextItem());
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetSeriesVisibility(const char* seriesname, bool visible)
{
  assert(seriesname != NULL);
  this->Internals->SeriesVisibilities[seriesname] = visible;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLineThickness(const char* seriesname, int value)
{
  assert(seriesname != NULL);
  this->Internals->LineThicknesses[seriesname] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLineStyle(const char* seriesname, int value)
{
  assert(seriesname != NULL);
  this->Internals->LineStyles[seriesname] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetColor(const char* seriesname, double r, double g, double b)
{
  assert(seriesname != NULL);
  this->Internals->Colors[seriesname] = vtkColor3d(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetUseColorMapping(const char* seriesname, bool useColorMapping)
{
  assert(seriesname != NULL);
  this->Internals->UseColorMapping[seriesname] = useColorMapping;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLookupTable(const char* seriesname, vtkScalarsToColors* lut)
{
  assert(seriesname != NULL);
  this->Internals->Lut[seriesname] = lut;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetAxisCorner(const char* seriesname, int corner)
{
  assert(seriesname != NULL);
  this->Internals->AxisCorners[seriesname] = corner;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetMarkerStyle(const char* seriesname, int style)
{
  assert(seriesname != NULL);
  this->Internals->MarkerStyles[seriesname] = style;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLabel(const char* seriesname, const char* label)
{
  assert(seriesname != NULL && label != NULL);
  this->Internals->Labels[seriesname] = label;
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkXYChartRepresentation::GetLabel(const char* seriesname) const
{
  assert(seriesname != NULL);
  return (this->Internals->Labels.find(seriesname) != this->Internals->Labels.end())
    ? this->Internals->Labels[seriesname].c_str()
    : NULL;
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearLineThicknesses()
{
  this->Internals->LineThicknesses.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearLineStyles()
{
  this->Internals->LineStyles.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearColors()
{
  this->Internals->Colors.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearAxisCorners()
{
  this->Internals->AxisCorners.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearMarkerStyles()
{
  this->Internals->MarkerStyles.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearLabels()
{
  this->Internals->Labels.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXYChartRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  if (!this->LocalOutput)
  {
    return 1;
  }

  this->PlotDataHasChanged = true;
  return 1;
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartXY* chartXY = this->GetChart();
  assert(chartXY != NULL); // we are assured this is always so.

  vtkChartRepresentation::MapOfTables tables;
  if (!this->GetLocalOutput(tables))
  {
    this->Internals->HideAllPlots();
    return;
  }

  if (this->UseIndexForXAxis == false &&
    (this->XAxisSeriesName == NULL || this->XAxisSeriesName[0] == 0))
  {
    vtkErrorMacro("Missing XAxisSeriesName.");
    this->Internals->HideAllPlots();
    return;
  }

  this->PlotDataHasChanged = false;

  if (this->GetChartType() == vtkChart::FUNCTIONALBAG)
  {
    chartXY->SetSelectionMethod(vtkChart::SELECTION_COLUMNS);
  }
  chartXY->SetSelectionMethod(this->GetChartType() == vtkChart::FUNCTIONALBAG
      ? vtkChart::SELECTION_COLUMNS
      : vtkChart::SELECTION_ROWS);
  // Update plots. This will create new vtkPlot if needed.
  this->Internals->UpdatePlots(this, tables);
  this->Internals->UpdatePlotProperties(this);
  assert(this->UseIndexForXAxis == true || this->XAxisSeriesName != NULL);
}

//----------------------------------------------------------------------------
bool vtkXYChartRepresentation::Export(vtkCSVExporter* exporter)
{
  assert(this->GetVisibility() == true);
  return this->Internals->Export(this, exporter);
}
