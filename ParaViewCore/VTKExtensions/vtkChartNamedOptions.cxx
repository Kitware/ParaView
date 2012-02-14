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
#include "vtkPlot.h"
#include "vtkTable.h"
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
  int Visible;

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
    this->ChartType = vtkChart::POINTS;
    this->TableVisibility = false;
  }

  PlotMapType PlotMap;
  int ChartType;
  bool TableVisibility;

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartNamedOptions)

//----------------------------------------------------------------------------
vtkChartNamedOptions::vtkChartNamedOptions()
{
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
  plotInfo.Visible = visible;
  plotInfo.VisibilityInitialized = true;
  this->SetPlotVisibilityInternal(plotInfo, visible !=0, name);
}

//----------------------------------------------------------------------------
int vtkChartNamedOptions::GetVisibility(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Visible? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetTableVisibility(bool visible)
{
  this->Internals->TableVisibility = visible;

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
  this->Internals->ChartType = type;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkChartNamedOptions::GetChartType()
{
  return this->Internals->ChartType;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetChart(vtkChart* chart)
{
  if (this->Internals->Chart == chart)
    {
    return;
    }
  this->Internals->Chart = chart;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkChart * vtkChartNamedOptions::GetChart()
{
  return this->Internals->Chart;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::SetTable(vtkTable* table)
{
  if (this->Internals->Table == table)
    {
    if (table && table->GetMTime() < this->RefreshTime)
      {
      return;
      }
    }

  this->Internals->Table = table;
  this->RefreshPlots();
  this->SetTableVisibility(this->Internals->TableVisibility);
  this->RefreshTime.Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkTable* vtkChartNamedOptions::GetTable()
{
  return this->Internals->Table;
}

//----------------------------------------------------------------------------
void vtkChartNamedOptions::RemovePlotsFromChart()
{
  if (!this->Internals->Chart)
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
  if (!this->Internals->Table)
    {
    return;
    }

  PlotMapType newMap;

  int defaultVisible = 1;

  // For each series (column in the table)
  const vtkIdType numberOfColumns = this->Internals->Table->GetNumberOfColumns();
  for (vtkIdType i = 0; i < numberOfColumns; ++i)
    {
    // Get the series name
    const char* seriesName = this->Internals->Table->GetColumnName(i);
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
  if (this->Internals->Chart)
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
void vtkChartNamedOptions::SetPlotVisibilityInternal(PlotInfo& plotInfo,
                                                     bool visible,
                                                     const char* seriesName)
{
  // FIXME: Do something sane for the base chart.
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
