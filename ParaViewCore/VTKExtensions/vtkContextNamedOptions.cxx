/*=========================================================================

  Program:   ParaView
  Module:    vtkContextNamedOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContextNamedOptions.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkPlotLine.h"
#include "vtkAxis.h"
#include "vtkPen.h"
#include "vtkTable.h"
#include "vtkColorSeries.h"
#include "vtkStdString.h"
#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"

#include "vtkstd/map"
#include "vtksys/ios/sstream"

//----------------------------------------------------------------------------
// Class to store information about each plot.
// A PlotInfo is created for each column in the vtkTable
class vtkContextNamedOptions::PlotInfo
{
public:
  vtkWeakPointer<vtkPlot> Plot;
  vtkStdString Label;
  bool ColorInitialized;
  bool VisibilityInitialized;
  int LineThickness;
  int LineStyle;
  int MarkerStyle;
  int Visible;
  int Corner;
  double Color[3];

  PlotInfo()
    {
    this->ColorInitialized = false;
    this->VisibilityInitialized = false;
    this->LineThickness = 2;
    this->LineStyle = 1;
    this->MarkerStyle = 0;
    this->Visible = 1;
    this->Corner = 0;
    this->Color[0] = this->Color[1] = this->Color[2] = 0;
    }

  PlotInfo(const PlotInfo &p)
    {
    this->ColorInitialized = p.ColorInitialized;
    this->VisibilityInitialized = p.VisibilityInitialized;
    this->LineThickness = p.LineThickness;
    this->LineStyle = p.LineStyle;
    this->MarkerStyle = p.MarkerStyle;
    this->Visible = p.Visible;
    this->Label = p.Label;
    this->Color[0] = p.Color[0];
    this->Color[1] = p.Color[1];
    this->Color[2] = p.Color[2];
    this->Plot = p.Plot;
    }

};

typedef vtkstd::map<vtkstd::string, vtkContextNamedOptions::PlotInfo > PlotMapType;
typedef PlotMapType::iterator PlotMapIterator;

//----------------------------------------------------------------------------
class vtkContextNamedOptions::vtkInternals
{
public:
  vtkInternals()
  {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->UseIndexForXAxis = true;
    this->ChartType = vtkChart::LINE;
    this->TableVisibility = false;
  }

  PlotMapType PlotMap;
  vtkstd::string XSeriesName;
  bool UseIndexForXAxis;
  int ChartType;
  bool TableVisibility;

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;
  vtkSmartPointer<vtkColorSeries> Colors;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContextNamedOptions);

//----------------------------------------------------------------------------
vtkContextNamedOptions::vtkContextNamedOptions()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkContextNamedOptions::~vtkContextNamedOptions()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetChartType(int type)
{
  this->Internals->ChartType = type;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetChartType()
{
  return this->Internals->ChartType;
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetChart(vtkChart* chart)
{
  if (this->Internals->Chart == chart)
    {
    return;
    }
  this->Internals->Chart = chart;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkChart * vtkContextNamedOptions::GetChart()
{
  return this->Internals->Chart;
}

//----------------------------------------------------------------------------
vtkTable* vtkContextNamedOptions::GetTable()
{
  return this->Internals->Table;
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetTable(vtkTable* table)
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
const char* vtkContextNamedOptions::GetXSeriesName()
{
  return this->Internals->UseIndexForXAxis? NULL :
    this->Internals->XSeriesName.c_str();
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetXSeriesName(const char* name)
{
  if (!name)
    {
    this->Internals->XSeriesName = "";
    }
  else
    {
    this->Internals->XSeriesName = name;
    }

  // Now update any existing plots to use the X series specified
  vtkstd::map<vtkstd::string, PlotInfo>::iterator it;
  for (it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    if (it->second.Plot)
      {
      it->second.Plot->SetInputArray(0, this->Internals->XSeriesName.c_str());
      it->second.Plot->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
      }
    }

  // If the X series column is changed, the range will have changed
  if (this->Internals->Chart)
    {
    this->Internals->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetUseIndexForXAxis(bool useIndex)
{
  this->Internals->UseIndexForXAxis = useIndex;
  // Now update the plots to use the X series specified
  vtkstd::map<vtkstd::string, PlotInfo>::iterator it;
  for (it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    if (it->second.Plot)
      {
      it->second.Plot->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
      }
    }

  // If the X series column is changed, the range will have changed
  if (this->Internals->Chart)
    {
    this->Internals->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------

namespace {
// Helper method to set color on PlotInfo
void SetPlotInfoColor(vtkContextNamedOptions::PlotInfo& plotInfo, vtkColor3ub color)
{
  plotInfo.Color[0] = color.GetData()[0]/255.0;
  plotInfo.Color[1] = color.GetData()[1]/255.0;
  plotInfo.Color[2] = color.GetData()[2]/255.0;
  plotInfo.ColorInitialized = true;
}
}


//----------------------------------------------------------------------------
void vtkContextNamedOptions::RefreshPlots()
{
  if (!this->Internals->Table)
    {
    return;
    }

  PlotMapType newMap;

  int defaultVisible = 1;
  if (strcmp(this->Internals->XSeriesName.c_str(), "bin_extents") == 0)
    {
    defaultVisible = 0;
    }

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
    if (!plotInfo.ColorInitialized)
      {
      SetPlotInfoColor(plotInfo, this->Internals->Colors->GetColorRepeating(i));
      }
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
        this->Internals->Chart->RemovePlotInstance(it->second.Plot);
        }
      }
    }

  this->Internals->PlotMap = newMap;
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::RemovePlotsFromChart()
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
      vtkPlot* plot = plotInfo.Plot;
      plotInfo.Plot = 0; // clear the weak pointer before destroying the plot
      this->Internals->Chart->RemovePlotInstance(plot);
      }
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetPlotVisibilityInternal(PlotInfo& plotInfo,
                                                       bool visible,
                                                       const char* seriesName)
{
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetVisible(static_cast<bool>(visible));
    }
  else if (this->Internals->Chart && this->Internals->Table && visible)
    {
    // Create a new vtkPlot and initialize it
    vtkPlot *plot = this->Internals->Chart->AddPlot(this->Internals->ChartType);
    if (plot)
      {
      plotInfo.Plot = plot;
      plot->SetVisible(static_cast<bool>(visible));
      plot->SetLabel(plotInfo.Label);
      plot->SetWidth(plotInfo.LineThickness);
      plot->GetPen()->SetLineType(plotInfo.LineStyle);
      plot->SetColor(plotInfo.Color[0], plotInfo.Color[1], plotInfo.Color[2]);
      // Must downcast to set the marker style...
      vtkPlotLine *line = vtkPlotLine::SafeDownCast(plot);
      if (line)
        {
        line->SetMarkerStyle(plotInfo.MarkerStyle);
        }
      plot->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
      plot->SetInput(this->Internals->Table,
                      this->Internals->XSeriesName.c_str(),
                      seriesName);
      }
    }
}

//----------------------------------------------------------------------------
vtkContextNamedOptions::PlotInfo&
vtkContextNamedOptions::GetPlotInfo(const char* seriesName)
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
void vtkContextNamedOptions::SetTableVisibility(bool visible)
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
void vtkContextNamedOptions::SetVisibility(const char* name, int visible)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Visible = visible;
  plotInfo.VisibilityInitialized = true;
  this->SetPlotVisibilityInternal(plotInfo, visible !=0, name);
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetLineThickness(const char* name,
                                                     int value)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineThickness = value;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetLineStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineStyle = style;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->GetPen()->SetLineType(style);
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetColor(const char* name,
                                             double r, double g, double b)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Color[0] = r;
  plotInfo.Color[1] = g;
  plotInfo.Color[2] = b;
  plotInfo.ColorInitialized = true;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetAxisCorner(const char* name, int value)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Corner = value;
  if (plotInfo.Plot && this->Internals->Chart)
    {
    vtkChartXY *chart = vtkChartXY::SafeDownCast(this->Internals->Chart);
    if (chart)
      {
      chart->SetPlotCorner(plotInfo.Plot, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetMarkerStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.MarkerStyle = style;

  // Must downcast to set the marker style...
  vtkPlotLine *line = vtkPlotLine::SafeDownCast(plotInfo.Plot);
  if (line)
    {
    line->SetMarkerStyle(style);
    }
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::SetLabel(const char* name,
                                             const char* label)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Label = label;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetLabel(label);
    }
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetVisibility(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Visible? 1 : 0;
}

//----------------------------------------------------------------------------
const char* vtkContextNamedOptions::GetLabel(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Label.c_str();
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetLineThickness(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.LineThickness;
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetLineStyle(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.LineStyle;
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetMarkerStyle(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.MarkerStyle;
}

//----------------------------------------------------------------------------
int vtkContextNamedOptions::GetAxisCorner(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Corner;
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::GetColor(const char* name, double rgb[3])
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  rgb[0] = plotInfo.Color[0];
  rgb[1] = plotInfo.Color[1];
  rgb[2] = plotInfo.Color[2];
}

//----------------------------------------------------------------------------
void vtkContextNamedOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
