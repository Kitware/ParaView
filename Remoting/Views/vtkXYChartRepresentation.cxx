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

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVXYChartView.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkSortFieldData.h"
#include "vtkTableAlgorithm.h"

class vtkXYChartRepresentation::SortTableFilter : public vtkTableAlgorithm
{
private:
  char* ArrayToSortBy;

protected:
  SortTableFilter()
    : ArrayToSortBy(nullptr)
  {
  }
  ~SortTableFilter() override = default;

public:
  static SortTableFilter* New();
  int RequestData(
    vtkInformation*, vtkInformationVector** inVector, vtkInformationVector* outVector) override
  {
    vtkTable* in = vtkTable::GetData(inVector[0], 0);
    vtkTable* out = vtkTable::GetData(outVector, 0);
    if (!this->ArrayToSortBy)
    {
      vtkErrorMacro(<< "The array name to sort by must be set.");
      return 0;
    }
    out->DeepCopy(in);
    vtkSortFieldData::Sort(out->GetRowData(), this->ArrayToSortBy, 0, 0);
    return 1;
  }

  vtkGetStringMacro(ArrayToSortBy);
  vtkSetStringMacro(ArrayToSortBy);
};

vtkStandardNewMacro(vtkXYChartRepresentation::SortTableFilter);

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
  , XAxisSeriesName(nullptr)
  , UseIndexForXAxis(true)
  , SortDataByXAxis(false)
  , PlotDataHasChanged(false)
  , SeriesLabelPrefix(nullptr)
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
  this->Internals = nullptr;
  this->SetXAxisSeriesName(nullptr);
  this->SetSeriesLabelPrefix(nullptr);
}

//----------------------------------------------------------------------------
bool vtkXYChartRepresentation::RemoveFromView(vtkView* view)
{
  if ((this->ContextView.GetPointer() == view) && (this->GetChart() != nullptr))
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
void vtkXYChartRepresentation::SetSortDataByXAxis(bool val)
{
  if (this->SortDataByXAxis == val)
  {
    return;
  }
  this->SortDataByXAxis = val;
  this->MarkModified();
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
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetSeriesVisibility(const char* seriesname, bool visible)
{
  assert(seriesname != nullptr);
  this->Internals->SeriesVisibilities[seriesname] = visible;
  this->Internals->SeriesOrder[seriesname] = static_cast<int>(this->Internals->SeriesOrder.size());
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLineThickness(const char* seriesname, double value)
{
  assert(seriesname != nullptr);
  this->Internals->LineThicknesses[seriesname] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLineStyle(const char* seriesname, int value)
{
  assert(seriesname != nullptr);
  this->Internals->LineStyles[seriesname] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetColor(const char* seriesname, double r, double g, double b)
{
  assert(seriesname != nullptr);
  this->Internals->Colors[seriesname] = vtkColor3d(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetUseColorMapping(const char* seriesname, bool useColorMapping)
{
  assert(seriesname != nullptr);
  this->Internals->UseColorMapping[seriesname] = useColorMapping;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLookupTable(const char* seriesname, vtkScalarsToColors* lut)
{
  assert(seriesname != nullptr);
  this->Internals->Lut[seriesname] = lut;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetAxisCorner(const char* seriesname, int corner)
{
  assert(seriesname != nullptr);
  this->Internals->AxisCorners[seriesname] = corner;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetMarkerStyle(const char* seriesname, int style)
{
  assert(seriesname != nullptr);
  this->Internals->MarkerStyles[seriesname] = style;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetMarkerSize(const char* seriesname, double value)
{
  assert(seriesname != nullptr);
  this->Internals->MarkerSizes[seriesname] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetLabel(const char* seriesname, const char* label)
{
  assert(seriesname != nullptr && label != nullptr);
  this->Internals->Labels[seriesname] = label;
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkXYChartRepresentation::GetLabel(const char* seriesname) const
{
  assert(seriesname != nullptr);
  return (this->Internals->Labels.find(seriesname) != this->Internals->Labels.end())
    ? this->Internals->Labels[seriesname].c_str()
    : nullptr;
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Internals->SeriesOrder.clear();
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
void vtkXYChartRepresentation::ClearMarkerSizes()
{
  this->Internals->MarkerSizes.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::ClearLabels()
{
  this->Internals->Labels.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXYChartRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVXYChartView* view = vtkPVXYChartView::SafeDownCast(inInfo->Get(vtkPVView::VIEW()));
    if (view)
    {
      this->SetSortDataByXAxis(view->GetSortByXAxis());
    }
  }
  return Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
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
vtkSmartPointer<vtkDataObject> vtkXYChartRepresentation::TransformTable(
  vtkSmartPointer<vtkDataObject> data)
{
  if (!(this->SortDataByXAxis && this->XAxisSeriesName))
  {
    return Superclass::TransformTable(data);
  }
  vtkNew<SortTableFilter> sorter;
  sorter->SetInputDataObject(data);
  sorter->SetArrayToSortBy(this->XAxisSeriesName);
  sorter->Update();
  vtkSmartPointer<vtkDataObject> sortedTable = sorter->GetOutputDataObject(0);
  return sortedTable;
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartXY* chartXY = this->GetChart();
  assert(chartXY != nullptr); // we are assured this is always so.

  vtkChartRepresentation::MapOfTables tables;
  if (!this->GetLocalOutput(tables))
  {
    this->Internals->HideAllPlots();
    return;
  }

  if (this->UseIndexForXAxis == false &&
    (this->XAxisSeriesName == nullptr || this->XAxisSeriesName[0] == 0))
  {
    vtkErrorMacro("Missing XAxisSeriesName.");
    this->Internals->HideAllPlots();
    return;
  }

  this->PlotDataHasChanged = false;
  chartXY->SetSelectionMethod(this->GetChartType() == vtkChart::FUNCTIONALBAG
      ? vtkChart::SELECTION_COLUMNS
      : vtkChart::SELECTION_ROWS);
  // Update plots. This will create new vtkPlot if needed.
  this->Internals->UpdatePlots(this, tables);
  this->Internals->UpdatePlotProperties(this);
  assert(this->UseIndexForXAxis == true || this->XAxisSeriesName != nullptr);
}

//----------------------------------------------------------------------------
bool vtkXYChartRepresentation::Export(vtkCSVExporter* exporter)
{
  assert(this->GetVisibility() == true);
  return this->Internals->Export(this, exporter);
}
