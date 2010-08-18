/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextNamedOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextNamedOptionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkSMStringVectorProperty.h"
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
class vtkSMContextNamedOptionsProxy::PlotInfo
{
public:
  vtkWeakPointer<vtkPlot> Plot;
  vtkStdString Label;
  bool ColorInitialized;
  int LineThickness;
  int LineStyle;
  int MarkerStyle;
  int Visible;
  int Corner;
  double Color[3];

  PlotInfo()
    {
    ColorInitialized = false;
    LineThickness = 2;
    LineStyle = 1;
    MarkerStyle = 0;
    Visible = 1;
    Corner = 0;
    Color[0] = Color[1] = Color[2] = 0;
    }

  PlotInfo(const PlotInfo &p)
    {
    ColorInitialized = p.ColorInitialized;
    LineThickness = p.LineThickness;
    LineStyle = p.LineStyle;
    MarkerStyle = p.MarkerStyle;
    Visible = p.Visible;
    Label = p.Label;
    Color[0] = p.Color[0];
    Color[1] = p.Color[1];
    Color[2] = p.Color[2];
    Plot = p.Plot;
    }

};

typedef vtkstd::map<vtkstd::string, vtkSMContextNamedOptionsProxy::PlotInfo > PlotMapType;
typedef PlotMapType::iterator PlotMapIterator;

//----------------------------------------------------------------------------
class vtkSMContextNamedOptionsProxy::vtkInternals
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
vtkStandardNewMacro(vtkSMContextNamedOptionsProxy);

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::vtkSMContextNamedOptionsProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::~vtkSMContextNamedOptionsProxy()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetChartType(int type)
{
  this->Internals->ChartType = type;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkSMContextNamedOptionsProxy::GetChartType()
{
  return this->Internals->ChartType;
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetChart(vtkChart* chart)
{
  if (this->Internals->Chart == chart)
    {
    return;
    }
  this->Internals->Chart = chart;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetTable(vtkTable* table)
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
void vtkSMContextNamedOptionsProxy::SetXSeriesName(const char* name)
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
void vtkSMContextNamedOptionsProxy::SetUseIndexForXAxis(bool useIndex)
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
void SetPlotInfoColor(vtkSMContextNamedOptionsProxy::PlotInfo& plotInfo, vtkColor3ub color)
{
  plotInfo.Color[0] = color.GetData()[0]/255.0;
  plotInfo.Color[1] = color.GetData()[1]/255.0;
  plotInfo.Color[2] = color.GetData()[2]/255.0;
  plotInfo.ColorInitialized = true;
}
}


//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::RefreshPlots()
{
  if (!this->Internals->Table)
    {
    return;
    }

  PlotMapType newMap;

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
void vtkSMContextNamedOptionsProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp || !svp->GetInformationOnly() || !this->Internals->Table)
    {
    return;
    }

  bool skip = false;
  const char* propertyName = this->GetPropertyName(prop);
  vtkSmartPointer<vtkStringList> newValues = vtkSmartPointer<vtkStringList>::New();

  // Note: we could iterate over this->Internals->PlotMap, but just for
  // kicks we'll iterate over the table columns in order to respect the
  // column ordering, probably not needed...
  int numberOfColumns = this->Internals->Table->GetNumberOfColumns();
  for (int i = 0; i < numberOfColumns; ++i)
    {
    const char* seriesName = this->Internals->Table->GetColumnName(i);
    if (!seriesName)
      {
      continue;
      }

    PlotInfo& plotInfo = this->GetPlotInfo(seriesName);
    newValues->AddString(seriesName);

    if (strcmp(propertyName, "VisibilityInfo") == 0)
      {
      newValues->AddString(plotInfo.Visible ? "1" : "0");
      }
    else if (strcmp(propertyName, "LabelInfo") == 0)
      {
      newValues->AddString(plotInfo.Label);
      }
    else if (strcmp(propertyName, "LineThicknessInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << plotInfo.LineThickness;
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "ColorInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      for (int i = 0; i < 3; ++i)
        {
        string << plotInfo.Color[i];
        newValues->AddString(string.str().c_str());
        string.str("");
        }
      }
    else if (strcmp(propertyName, "LineStyleInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << plotInfo.LineStyle;
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "MarkerStyleInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << plotInfo.MarkerStyle;
      newValues->AddString(string.str().c_str());
      }
    else if (strcmp(propertyName, "PlotCornerInfo") == 0)
      {
      vtksys_ios::ostringstream string;
      string << plotInfo.Corner;
      newValues->AddString(string.str().c_str());
      }
    else
      {
      skip = true;
      break;
      }
    }
  if (!skip)
    {
    svp->SetElements(newValues);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::RemovePlotsFromChart()
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
void vtkSMContextNamedOptionsProxy::SetPlotVisibilityInternal(PlotInfo& plotInfo,
                                          bool visible, const char* seriesName)
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

  // Recalculate the bounds if a plot was made visible
  if (this->Internals->Chart && visible)
    {
    this->Internals->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::PlotInfo&
vtkSMContextNamedOptionsProxy::GetPlotInfo(const char* seriesName)
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
void vtkSMContextNamedOptionsProxy::SetTableVisibility(bool visible)
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
void vtkSMContextNamedOptionsProxy::SetVisibility(const char* name, int visible)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.Visible = visible;
  this->SetPlotVisibilityInternal(plotInfo, visible, name);
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLineThickness(const char* name,
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
void vtkSMContextNamedOptionsProxy::SetLineStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->GetPlotInfo(name);
  plotInfo.LineStyle = style;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->GetPen()->SetLineType(style);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetColor(const char* name,
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
void vtkSMContextNamedOptionsProxy::SetAxisCorner(const char* name, int value)
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
void vtkSMContextNamedOptionsProxy::SetMarkerStyle(const char* name, int style)
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
void vtkSMContextNamedOptionsProxy::SetLabel(const char* name,
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
void vtkSMContextNamedOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
