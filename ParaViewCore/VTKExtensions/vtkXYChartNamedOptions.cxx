/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartNamedOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXYChartNamedOptions.h"

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
#include "vtkNew.h"

#include <map>
#include <sstream>

//----------------------------------------------------------------------------
// Class to store information about each plot.
// A PlotInfo is created for each column in the vtkTable
class vtkXYChartNamedOptions::PlotInfo
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

typedef std::map<std::string, vtkXYChartNamedOptions::PlotInfo > PlotMapType;
typedef PlotMapType::iterator PlotMapIterator;

//----------------------------------------------------------------------------
class vtkXYChartNamedOptions::vtkInternals
{
public:
  vtkInternals()
  {
    this->UseIndexForXAxis = true;
  }

  PlotMapType PlotMap;
  std::string XSeriesName;
  bool UseIndexForXAxis;

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;
  vtkNew<vtkColorSeries> Colors;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXYChartNamedOptions);

//----------------------------------------------------------------------------
vtkXYChartNamedOptions::vtkXYChartNamedOptions()
{
  this->ChartType = vtkChart::LINE;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkXYChartNamedOptions::~vtkXYChartNamedOptions()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetChartType(int type)
{
  this->ChartType = type;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXYChartNamedOptions::GetChartType()
{
  return this->ChartType;
}

//----------------------------------------------------------------------------
const char* vtkXYChartNamedOptions::GetXSeriesName()
{
  return this->Internals->UseIndexForXAxis? NULL :
    this->Internals->XSeriesName.c_str();
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetXSeriesName(const char* name)
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
  std::map<std::string, PlotInfo>::iterator it;
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
  if (this->Chart)
    {
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetUseIndexForXAxis(bool useIndex)
{
  this->Internals->UseIndexForXAxis = useIndex;
  // Now update the plots to use the X series specified
  std::map<std::string, PlotInfo>::iterator it;
  for (it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    if (it->second.Plot)
      {
      it->second.Plot->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
      }
    }

  // If the X series column is changed, the range will have changed
  if (this->Chart)
    {
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------

namespace {
// Helper method to set color on PlotInfo
void SetPlotInfoColor(vtkXYChartNamedOptions::PlotInfo& plotInfo, vtkColor3ub color)
{
  plotInfo.Color[0] = color.GetData()[0]/255.0;
  plotInfo.Color[1] = color.GetData()[1]/255.0;
  plotInfo.Color[2] = color.GetData()[2]/255.0;
  plotInfo.ColorInitialized = true;
}
}


//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::RefreshPlots()
{
  if (!this->Table)
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
  if (this->Chart)
    {
    PlotMapIterator it = this->Internals->PlotMap.begin();
    for ( ; it != this->Internals->PlotMap.end(); ++it)
      {
      // If the series is currently in the chart but has been removed from
      // the vtkTable then lets remove it from the chart
      if (it->second.Plot && newMap.find(it->first) == newMap.end())
        {
        this->Chart->RemovePlotInstance(it->second.Plot);
        }
      }
    }

  this->Internals->PlotMap = newMap;
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::RemovePlotsFromChart()
{
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
      vtkPlot* plot = plotInfo.Plot;
      plotInfo.Plot = 0; // clear the weak pointer before destroying the plot
      this->Chart->RemovePlotInstance(plot);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetPlotVisibilityInternal(PlotInfo& plotInfo,
                                                       bool visible,
                                                       const char* seriesName)
{
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetVisible(static_cast<bool>(visible));
    }
  else if (this->Chart && this->Table && visible)
    {
    // Create a new vtkPlot and initialize it
    vtkPlot *plot = this->Chart->AddPlot(this->ChartType);
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
      plot->SetInputData(this->Table,
                         this->Internals->XSeriesName.c_str(),
                         seriesName);
      }
    }
}

//----------------------------------------------------------------------------
vtkXYChartNamedOptions::PlotInfo&
vtkXYChartNamedOptions::GetPlotInfo(const char* seriesName)
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
void vtkXYChartNamedOptions::SetTableVisibility(bool visible)
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
void vtkXYChartNamedOptions::SetVisibility(const char* name, int visible)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Visible = visible;
  plotInfo.VisibilityInitialized = true;
  this->SetPlotVisibilityInternal(plotInfo, visible !=0, name);
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetLineThickness(const char* name,
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
void vtkXYChartNamedOptions::SetLineStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineStyle = style;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->GetPen()->SetLineType(style);
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetColor(const char* name,
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
void vtkXYChartNamedOptions::SetAxisCorner(const char* name, int value)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Corner = value;
  if (plotInfo.Plot && this->Chart)
    {
    vtkChartXY *chart = vtkChartXY::SafeDownCast(this->Chart);
    if (chart)
      {
      chart->SetPlotCorner(plotInfo.Plot, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetMarkerStyle(const char* name, int style)
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
void vtkXYChartNamedOptions::SetLabel(const char* name,
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
int vtkXYChartNamedOptions::GetVisibility(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Visible? 1 : 0;
}

//----------------------------------------------------------------------------
const char* vtkXYChartNamedOptions::GetLabel(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Label.c_str();
}

//----------------------------------------------------------------------------
int vtkXYChartNamedOptions::GetLineThickness(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.LineThickness;
}

//----------------------------------------------------------------------------
int vtkXYChartNamedOptions::GetLineStyle(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.LineStyle;
}

//----------------------------------------------------------------------------
int vtkXYChartNamedOptions::GetMarkerStyle(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.MarkerStyle;
}

//----------------------------------------------------------------------------
int vtkXYChartNamedOptions::GetAxisCorner(const char* name)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  return plotInfo.Corner;
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::GetColor(const char* name, double rgb[3])
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  rgb[0] = plotInfo.Color[0];
  rgb[1] = plotInfo.Color[1];
  rgb[2] = plotInfo.Color[2];
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
