/*=========================================================================

  Program:   ParaView
  Module:    vtkChartNamedOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChartNamedOptions.h"

#include "vtkObjectFactory.h"
#include "vtkChart.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkStdString.h"
#include "vtkWeakPointer.h"
#include "vtkNew.h"

#include <map>
#include <sstream>

//----------------------------------------------------------------------------
// Class to store information about each plot.
// A PlotInfo is created for each column in the vtkTable
class vtkChartNamedOptions::PlotInfo
{
public:
  vtkWeakPointer<vtkPlot> Plot;
  vtkStdString Label;
  bool VisibilityInitialized;
  bool Visible;

  PlotInfo() : VisibilityInitialized(false), Visible(true)
    {
    }

  PlotInfo(const PlotInfo &p)
    {
    this->VisibilityInitialized = p.VisibilityInitialized;
    this->Visible = p.Visible;
    this->Label = p.Label;
    this->Plot = p.Plot;
    }
};

typedef std::map<std::string, vtkChartNamedOptions::PlotInfo> PlotMapType;
typedef PlotMapType::iterator PlotMapIterator;

//----------------------------------------------------------------------------
class vtkChartNamedOptions::vtkInternals
{
public:
  vtkInternals()
  {
  }

  PlotMapType PlotMap;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartNamedOptions)

//----------------------------------------------------------------------------
vtkChartNamedOptions::vtkChartNamedOptions()
{
  this->ChartType = vtkChart::POINTS;
  this->TableVisibility = false;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkChartNamedOptions::~vtkChartNamedOptions()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetVisibility(const char* name, int visible)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Visible = visible != 0;
  plotInfo.VisibilityInitialized = true;
  this->SetPlotVisibilityInternal(plotInfo, visible != 0, name);
}

//----------------------------------------------------------------------------
int vtkChartNamedOptions::GetVisibility(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Visible ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetLabel(const char* name, const char* label)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Label = label;
}

//----------------------------------------------------------------------------
const char* vtkChartNamedOptions::GetLabel(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Label.c_str();
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetTableVisibility(bool visible)
{
  this->TableVisibility = visible;

  for (PlotMapIterator it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    PlotInfo& plotInfo = it->second;
    this->SetPlotVisibilityInternal(plotInfo, visible && plotInfo.Visible,
                                    it->first.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetChartType(int type)
{
  this->ChartType = type;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkChartNamedOptions::GetChartType()
{
  return this->ChartType;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetChart(vtkChart* chart)
{
  if (this->Chart == chart)
    {
    return;
    }
  this->Chart = chart;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkChart * vtkChartNamedOptions::GetChart()
{
  return this->Chart;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetPlotMatrix(vtkScatterPlotMatrix* plotmatrix)
{
  if (this->PlotMatrix == plotmatrix)
    {
    return;
    }
  this->PlotMatrix = plotmatrix;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkScatterPlotMatrix * vtkChartNamedOptions::GetPlotMatrix()
{
  return this->PlotMatrix;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::UpdatePlotOptions()
{
  for (PlotMapIterator it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    PlotInfo& plotInfo = it->second;
    this->SetPlotVisibilityInternal(plotInfo, plotInfo.Visible,
                                    it->first.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetTable(vtkTable* table)
{
  if (this->Table == table)
    {
    if (table && table->GetMTime() < this->RefreshTime)
      {
      return;
      }
    }

  this->Table = table;
  this->RefreshPlots();
  this->SetTableVisibility(this->TableVisibility);
  this->RefreshTime.Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkTable* vtkChartNamedOptions::GetTable()
{
  return this->Table;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::RemovePlotsFromChart()
{
  if(this->PlotMatrix)
    {
    this->PlotMatrix->SetVisibleColumns(NULL);
    }
  if (!this->Chart)
    {
    return;
    }

  for (PlotMapIterator it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    PlotInfo& plotInfo = it->second;
    if (plotInfo.Plot)
      {
      // FIXME: Do something sane here too...
      }
    }
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::RefreshPlots()
{
  if (!this->Table)
    {
    return;
    }

  PlotMapType newMap;

  int defaultVisible = 1;

  // For each series (column in the table)
  const vtkIdType numberOfColumns = this->Table->GetNumberOfColumns();
  for (vtkIdType i = 0; i < numberOfColumns; ++i)
    {
    // Get the series name
    const char* seriesName = this->Table->GetColumnName(i);
    if (!seriesName || !seriesName[0])
      {
      continue;
      }

    // Get the existing PlotInfo or initialize a new one
    PlotInfo& plotInfo = this->GetPlotInfo(seriesName);
    if (!plotInfo.VisibilityInitialized)
      {
      plotInfo.VisibilityInitialized = true;
      plotInfo.Visible = defaultVisible;
      }

    // Add the PlotInfo to the new collection
    newMap[seriesName] = plotInfo;
    }

  // Now we need to prune old series (table columns that were removed)
  if (this->Chart)
    {
    PlotMapIterator it = this->Internals->PlotMap.begin();
    for ( ; it != this->Internals->PlotMap.end(); ++it)
      {
      // If the series is currently in the chart but has been removed from
      // the vtkTable then lets remove it from the chart
      if (it->second.Plot && newMap.find(it->first) == newMap.end())
        {
        // FIXME: Do this on the string!
        }
      }
    }

  this->Internals->PlotMap = newMap;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetPlotVisibilityInternal(PlotInfo&,
                                                     bool visible,
                                                     const char* seriesName)
{
  if (this->Chart)
    {
    if( vtkChartParallelCoordinates* parallelCharts =
      vtkChartParallelCoordinates::SafeDownCast(this->Chart))
      {
      parallelCharts->SetColumnVisibility(seriesName, visible);
      }
    }
  else if(this->PlotMatrix)
    {
    this->PlotMatrix->SetColumnVisibility(seriesName, visible);
    }
}

//----------------------------------------------------------------------------
vtkChartNamedOptions::PlotInfo&
vtkChartNamedOptions::GetPlotInfo(const char* seriesName)
{
  PlotMapIterator it = this->Internals->PlotMap.find(seriesName);
  if (it != this->Internals->PlotMap.end())
    {
    return it->second;
    }
  else
    {
    PlotInfo& plotInfo = this->Internals->PlotMap[seriesName];
    plotInfo.Label = seriesName;
    return plotInfo;
    }
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
