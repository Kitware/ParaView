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

#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotPoints.h"
#include "vtkPVContextView.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>
class vtkXYChartRepresentation::vtkInternals
{
  struct PlotInfo
    {
    vtkSmartPointer<vtkPlot> Plot;
    std::string TableName;
    std::string SeriesName;
    };

  typedef std::map<std::string, PlotInfo> PlotsType;
  PlotsType Plots;

  //---------------------------------------------------------------------------
  // Makes is easy to obtain a value for a series parameter, is set, else the
  // default. This class supports two mechanisms for addresses series in a
  // collection  (multiblock) of tables: (1) using a name that combines the
  // table name and the column name (using
  // vtkChartRepresentation::GetDefaultSeriesLabel), or (2) using the column
  // name alone. (1) is always checked before (2).
  template <class T>
  T GetSeriesParameter(const std::string& tableName,
    const std::string& columnName,
    const std::map<std::string, T> &parameter_map,
    const T &default_value=T()) const
    {
    typename std::map<std::string, T>::const_iterator iter;

    // when setting properties for a series, I want to support two mechanisms:
    // simply specifying the array name or suffixing it with the block-name.
    // This logic makes that possible.

    // first try most specific form of identifying the series.
    std::string key = vtkChartRepresentation::GetDefaultSeriesLabel(
      tableName, columnName);
    iter = parameter_map.find(key);
    if (iter != parameter_map.end())
      {
      return iter->second;
      }

    // now try the cheap form for identifying it.
    key = columnName;
    iter = parameter_map.find(key);
    if (iter != parameter_map.end())
      {
      return iter->second;
      }
    return default_value;
    }

  //---------------------------------------------------------------------------
  // Returns a vtkPlot associated with a specific series, if any.
  vtkPlot* GetSeriesPlot(
    const std::string& tableName, const std::string& columnName) const
    {
    std::string key = tableName + ":" + columnName;
    PlotsType::const_iterator iter = this->Plots.find(key);
    return (iter != this->Plots.end()?
      iter->second.Plot.GetPointer() : NULL);
    }

  //---------------------------------------------------------------------------
  // Adds a new vtkPlot instance to the internal datastructure used by
  // GetSeriesPlot(..).
  void AddSeriesPlot(
    const std::string& tableName, const std::string& columnName, vtkPlot* plot)
    {
    assert(plot != NULL);
    std::string key = tableName + ":" + columnName;
    assert(this->Plots.find(key) == this->Plots.end());
    PlotInfo &info = this->Plots[key];
    info.Plot = plot;
    info.TableName = tableName;
    info.SeriesName = columnName;
    }

public:
  typedef vtkXYChartRepresentation::MapOfTables MapOfTables;

  // we have to keep these separate since they are set by different properties
  // and hence may not always match up.
  std::map<std::string, bool> SeriesVisibilities;
  std::map<std::string, int> LineThicknesses;
  std::map<std::string, int> LineStyles;
  std::map<std::string, vtkColor3d> Colors;
  std::map<std::string, int> AxisCorners;
  std::map<std::string, int> MarkerStyles;
  std::map<std::string, std::string> Labels;

  // These are used to determine when to recalculate chart bounds. If user
  // changes the X axis, we force recalculation of the chart bounds
  // automatically.
  bool PreviousUseIndexForXAxis;
  std::string PreviousXAxisSeriesName;

  vtkInternals()
    : PreviousUseIndexForXAxis(false)
    {
    }

  //---------------------------------------------------------------------------
  // Hide all plots.
  void HideAllPlots()
    {
    PlotsType::iterator iter;
    for (iter = this->Plots.begin(); iter != this->Plots.end(); ++iter)
      {
      iter->second.Plot->SetVisible(false);
      }
    }

  //---------------------------------------------------------------------------
  // Destroy all vtkPlot instances that don't correspond to any column in any of
  // the tables.
  void DestroyObsoletePlots(
    vtkChartXY* chartXY, const MapOfTables& tables)
    {
    PlotsType newPlots;
    for (MapOfTables::const_iterator iter = tables.begin(); iter != tables.end(); ++iter)
      {
      const std::string &tableName = iter->first;
      vtkTable* table = iter->second.GetPointer();

      vtkIdType numCols = table->GetNumberOfColumns();
      for (vtkIdType cc=0; cc < numCols; ++cc)
        {
        std::string key = tableName + ":" + table->GetColumnName(cc);
        PlotsType::iterator plotsIter = this->Plots.find(key);
        if (plotsIter != this->Plots.end())
          {
          newPlots[key] = plotsIter->second;
          this->Plots.erase(plotsIter);
          }
        }
      }

    // any plots remaining in this->Plots are no longer used. Remove them.
    for (PlotsType::iterator iter = this->Plots.begin();
      iter != this->Plots.end(); ++iter)
      {
      chartXY->RemovePlotInstance(iter->second.Plot);
      }
    this->Plots = newPlots;
    }

  //---------------------------------------------------------------------------
  // This method will add vtkPlot instance (create new ones if neeed) for each
  // of the columns in each of the "tables" for which we have a visibility flag
  // set to true.
  void UpdatePlots(vtkXYChartRepresentation* self, const MapOfTables& tables)
    {
    assert(self != NULL);

    vtkChartXY* chartXY = self->GetChart();
    for (MapOfTables::const_iterator tablesIter = tables.begin();
      tablesIter != tables.end(); ++tablesIter)
      {
      const std::string &tableName = tablesIter->first;
      vtkTable* table = tablesIter->second.GetPointer();

      vtkIdType numCols = table->GetNumberOfColumns();
      for (vtkIdType cc=0; cc < numCols; ++cc)
        {
        std::string columnName = table->GetColumnName(cc);
        if (!this->GetSeriesParameter(tableName, columnName, this->SeriesVisibilities, false))
          {
          // skip invisible series.
          if (vtkPlot* plot = this->GetSeriesPlot(tableName, columnName))
            {
            plot->SetVisible(false);
            }
          continue;
          }

        // Now, we know the series needs to be shown, so update the vtkPlot.
        vtkPlot* plot = this->GetSeriesPlot(tableName, columnName);
        if (!plot)
          {
          plot = chartXY->AddPlot(self->GetChartType());
          if (!plot)
            {
            vtkGenericWarningMacro("Failed to create new vtkPlot of type: " <<
              self->GetChartType());
            continue;
            }
          this->AddSeriesPlot(tableName, columnName, plot);
          }

        plot->SetVisible(true);

        std::string default_label = vtkChartRepresentation::GetDefaultSeriesLabel(
          tableName, columnName);
        plot->SetLabel(this->GetSeriesParameter(tableName, columnName,
            this->Labels, default_label));

        vtkColor3d color = this->GetSeriesParameter(tableName, columnName,
          this->Colors, vtkColor3d(0, 0, 0));
        plot->SetColor(color.GetRed(), color.GetGreen(), color.GetBlue());

        plot->SetWidth(this->GetSeriesParameter(tableName, columnName,
            this->LineThicknesses, 2));
        plot->GetPen()->SetLineType(this->GetSeriesParameter(tableName, columnName,
            this->LineStyles, static_cast<int>(vtkPen::SOLID_LINE)));

        if (vtkPlotPoints* plotPoints = vtkPlotPoints::SafeDownCast(plot))
          {
          plotPoints->SetMarkerStyle(
            this->GetSeriesParameter(tableName, columnName,
              this->MarkerStyles, static_cast<int>(vtkPlotPoints::NONE)));
          // the vtkValidPointMask array is used by some filters (like plot
          // over line) to indicate invalid points. this instructs the line
          // plot to not render those points
          plotPoints->SetValidPointMaskName("vtkValidPointMask");
          }
        plot->SetUseIndexForXSeries(self->GetUseIndexForXAxis());
        plot->SetInputData(table, self->GetXAxisSeriesName(), columnName);

        chartXY->SetPlotCorner(plot, this->GetSeriesParameter(tableName, columnName,
            this->AxisCorners, 0));
        }
      }
    }
};

vtkStandardNewMacro(vtkXYChartRepresentation);
//----------------------------------------------------------------------------
vtkXYChartRepresentation::vtkXYChartRepresentation()
  : Internals(new vtkXYChartRepresentation::vtkInternals()),
  ChartType(vtkChart::LINE),
  XAxisSeriesName(NULL),
  UseIndexForXAxis(true),
  PlotDataHasChanged(false)
{
}

//----------------------------------------------------------------------------
vtkXYChartRepresentation::~vtkXYChartRepresentation()
{
  if (this->GetChart())
    {
    this->Internals->DestroyObsoletePlots(this->GetChart(),
      vtkChartRepresentation::MapOfTables());
    }
  delete this->Internals;
  this->Internals = NULL;
  this->SetXAxisSeriesName(NULL);
}

//----------------------------------------------------------------------------
bool vtkXYChartRepresentation::RemoveFromView(vtkView* view)
{
  if ((this->ContextView.GetPointer() == view) && (this->GetChart() != NULL))
    {
    this->Internals->DestroyObsoletePlots(this->GetChart(),
      vtkChartRepresentation::MapOfTables());
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
  return (this->Internals->Labels.find(seriesname) !=
          this->Internals->Labels.end()) ?
    this->Internals->Labels[seriesname].c_str() : NULL;
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
int vtkXYChartRepresentation::RequestData(vtkInformation *request,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
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

  if (this->PlotDataHasChanged)
    {
    // Destroy obsolete vtkPlot instances that refer to columns that are no
    // longer present in the input dataset.
    this->Internals->DestroyObsoletePlots(chartXY, tables);
    }
  this->PlotDataHasChanged = false;

  // Update plots. This will create new vtkPlot if needed.
  this->Internals->UpdatePlots(this, tables);

  assert(this->UseIndexForXAxis == true || this->XAxisSeriesName != NULL);
}
