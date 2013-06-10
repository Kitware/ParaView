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
#include <vector>

//----------------------------------------------------------------------------
// Class to store information about each plot.
// A PlotInfo is created for each column in the vtkTable
class vtkXYChartNamedOptions::PlotInfo
{
public:
  std::vector<vtkWeakPointer<vtkPlot> > Plots;
  std::vector<vtkWeakPointer<vtkTable> > Tables;
  vtkStdString Label;
  vtkStdString SerieName;
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
    this->SerieName = p.SerieName;
    this->Color[0] = p.Color[0];
    this->Color[1] = p.Color[1];
    this->Color[2] = p.Color[2];
    this->Plots = p.Plots;
    this->Tables = p.Tables;
    this->Corner = p.Corner;
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
    for(size_t i=0; i<it->second.Plots.size();i++)
      {
      if(it->second.Plots[i])
        {
        it->second.Plots[i]->SetInputArray(0, this->Internals->XSeriesName.c_str());
        it->second.Plots[i]->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
        }
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
    for(size_t i=0; i<it->second.Plots.size();i++)
      {
      if(!it->second.Plots[i])
        {
        continue;
        }
      it->second.Plots[i]->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
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
  //remove any table that has been deleted
  std::vector<vtkWeakPointer<vtkTable> > tables;
  for(size_t tbIndex=0; tbIndex< this->Tables.size(); tbIndex++)
    {
    if(this->Tables[tbIndex])
      {
      tables.push_back(this->Tables[tbIndex]);
      }
    }
  this->Tables = tables;

  this->RemovePlotsFromChart();

  if(this->Tables.empty())
    {
    return;
    }

  PlotMapType newMap;

  int defaultVisible = 1;
  if (strcmp(this->Internals->XSeriesName.c_str(), "bin_extents") == 0)
    {
    defaultVisible = 0;
    }

  for(size_t tbIndex=0; tbIndex< this->Tables.size(); tbIndex++)
    {
    vtkTable* table = this->Tables[tbIndex];

    // For each series (column in the table)
    const vtkIdType numberOfColumns = table->GetNumberOfColumns();
    for (vtkIdType i = 0; i < numberOfColumns; ++i)
      {
      // Get the series name
      const char* seriesName = table->GetColumnName(i);
      if (!seriesName || !seriesName[0])
        {
        continue;
        }

      std::ostringstream fullSerieName;

      // Append block name only if blocks
      if(this->Tables.size() > 1)
        {
        fullSerieName << this->TableBlockNames[tbIndex] << "/";
        }

      fullSerieName << seriesName;

      // Get the existing PlotInfo or initialize a new one
      if(newMap.find(fullSerieName.str())==newMap.end())
        {
        PlotInfo& plotInfo = this->GetPlotInfo(fullSerieName.str().c_str());
        plotInfo.SerieName = seriesName;
        plotInfo.Tables.clear();
        plotInfo.Plots.clear();
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
        newMap[fullSerieName.str()] = plotInfo;
        }
      newMap[fullSerieName.str()].Tables.push_back(table);
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
    for(size_t i=0; i<plotInfo.Plots.size();i++)
      {
      vtkPlot* plot = plotInfo.Plots[i];
      plotInfo.Plots[i] = 0; // clear the weak pointer before destroying the plot
      this->Chart->RemovePlotInstance(plot);
      }
    plotInfo.Plots.clear();
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetPlotVisibilityInternal(PlotInfo& plotInfo,
                                                       bool visible,
                                                       const char* seriesName)
{
  if (plotInfo.Plots.size()>0)
    {
      for(size_t i=0; i<plotInfo.Plots.size();i++)
        {
        if(plotInfo.Plots[i])
          {
          plotInfo.Plots[i]->SetVisible(static_cast<bool>(visible));
          }
        }
    }
  else if (this->Chart && visible)
    {
    vtkChartXY *chartxy = vtkChartXY::SafeDownCast(this->Chart);
    for (size_t i = 0; i < plotInfo.Tables.size(); i++)
      {
      // Create a new vtkPlot and initialize it
      vtkPlot *plot = this->Chart->AddPlot(this->ChartType);
      if (plot)
        {
        plotInfo.Plots.push_back(plot);
        plot->SetVisible(static_cast<bool>(visible));
        plot->SetLabel(plotInfo.Label);
        plot->SetWidth(plotInfo.LineThickness);
        plot->GetPen()->SetLineType(plotInfo.LineStyle);
        plot->SetColor(plotInfo.Color[0], plotInfo.Color[1], plotInfo.Color[2]);
        chartxy->SetPlotCorner(plot, plotInfo.Corner);
        // Must downcast to set the marker style...
        vtkPlotLine *line = vtkPlotLine::SafeDownCast(plot);
        if (line)
          {
          line->SetMarkerStyle(plotInfo.MarkerStyle);

          // the vtkValidPointMask array is used by some filters (like plot
          // over line) to indicate invalid points. this instructs the line
          // plot to not render those points
          line->SetValidPointMaskName("vtkValidPointMask");
          }
        plot->SetUseIndexForXSeries(this->Internals->UseIndexForXAxis);
        plot->SetInputData(plotInfo.Tables[i],
                           this->Internals->XSeriesName.c_str(),
                           seriesName);
        }
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
    plotInfo.SerieName = seriesName;
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
                                    plotInfo.SerieName);
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetVisibility(const char* name, int visible)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Visible = visible;
  plotInfo.VisibilityInitialized = true;
  this->SetPlotVisibilityInternal(plotInfo, visible !=0, plotInfo.SerieName);
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetLineThickness(const char* name,
                                                     int value)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineThickness = value;
  for(size_t i=0; i<plotInfo.Plots.size();i++)
    {
    plotInfo.Plots[i]->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetLineStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineStyle = style;
  for(size_t i=0; i<plotInfo.Plots.size();i++)
    {
    plotInfo.Plots[i]->GetPen()->SetLineType(style);
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
  for(size_t i=0; i<plotInfo.Plots.size();i++)
    {
    plotInfo.Plots[i]->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetAxisCorner(const char* name, int value)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Corner = value;
  if (this->Chart)
    {
    vtkChartXY *chart = vtkChartXY::SafeDownCast(this->Chart);
    if (chart)
      {
      for (size_t i=0; i < plotInfo.Plots.size(); i++)
        {
        chart->SetPlotCorner(plotInfo.Plots[i], value);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetMarkerStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.MarkerStyle = style;

  // Must downcast to set the marker style...
  for(size_t i=0; i<plotInfo.Plots.size();i++)
    {
    vtkPlotLine *line = vtkPlotLine::SafeDownCast(plotInfo.Plots[i]);
    if (line)
      {
      line->SetMarkerStyle(style);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXYChartNamedOptions::SetLabel(const char* name,
                                             const char* label)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Label = label;
  for(size_t i=0; i<plotInfo.Plots.size();i++)
    {
    plotInfo.Plots[i]->SetLabel(label);
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
