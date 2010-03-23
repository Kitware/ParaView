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
#include "vtkChart.h"
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

#include <QString>

//----------------------------------------------------------------------------
// POD class to store information about each plot.
// A PlotInfo is created for each column in the vtkTable
class vtkSMContextNamedOptionsProxy::PlotInfo
{
public:
  vtkWeakPointer<vtkPlot> Plot;
  vtkStdString Label;
  int LineThickness;
  int LineStyle;
  int MarkerStyle;
  int Visible;
  double Color[3];

  PlotInfo()
    {
    LineThickness = 2;
    LineStyle = 1;
    MarkerStyle = 0;
    Visible = 1;
    Color[0] = Color[1] = Color[2] = 0;
    Color[0] = 1;
    }
};

typedef vtkstd::map<vtkstd::string, vtkSMContextNamedOptionsProxy::PlotInfo > PlotMapType;
typedef PlotMapType::iterator PlotMapIterator;

//----------------------------------------------------------------------------
class vtkSMContextNamedOptionsProxy::vtkInternals
{
public:
  PlotMapType PlotMap;
  vtkstd::string XSeriesName;
  bool UseIndexForXAxis;
  int ChartType;

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;
  vtkSmartPointer<vtkColorSeries> Colors;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMContextNamedOptionsProxy);
vtkCxxRevisionMacro(vtkSMContextNamedOptionsProxy, "1.22");

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::vtkSMContextNamedOptionsProxy()
{
  this->Internals = new vtkInternals();
  this->Internals->Colors = vtkSmartPointer<vtkColorSeries>::New();
  this->Internals->UseIndexForXAxis = true;
  this->Internals->ChartType = vtkChart::LINE;
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
    return;
    }
  this->Internals->Table = table;
  this->RefreshPlots();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::RefreshBottomAxisTitle()
{
  if (this->Internals->Chart)
    {
    if (this->Internals->UseIndexForXAxis)
      {
      this->Internals->Chart->GetAxis(vtkAxis::BOTTOM)->SetTitle("Index of Array");
      }
    else
      {
      this->Internals->Chart->GetAxis(vtkAxis::BOTTOM)
          ->SetTitle(this->Internals->XSeriesName.c_str());
      }
    this->Internals->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::RefreshLegendStatus()
{
  // Figure out how many active plot items there are - set the y axis title
  // or display legend to true.
  int active = 0;
  vtkPlot* lastPlot = 0;
  vtkstd::map<vtkstd::string, PlotInfo >::iterator it;
  for (it = this->Internals->PlotMap.begin();
       it != this->Internals->PlotMap.end(); ++it)
    {
    if (it->second.Plot && it->second.Visible)
      {
      ++active;
      lastPlot = it->second.Plot;
      }
    }
  if (active == 0 && this->Internals->Chart)
    {
    this->Internals->Chart->GetAxis(vtkAxis::LEFT)->SetTitle(" ");
    this->Internals->Chart->SetShowLegend(false);
    }
  else if (active == 1 && this->Internals->Chart)
    {
    this->Internals->Chart->GetAxis(vtkAxis::LEFT)->SetTitle(lastPlot->GetLabel());
    this->Internals->Chart->SetShowLegend(false);
    }
  else if (this->Internals->Chart)
    {
    this->Internals->Chart->GetAxis(vtkAxis::LEFT)->SetTitle(" ");
    this->Internals->Chart->SetShowLegend(true);
    }
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

  this->RefreshBottomAxisTitle();
  this->Modified();
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

  this->RefreshBottomAxisTitle();
  this->Modified();
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

    // Find an existing PlotInfo object or initialize a new one,
    // then add it to newMap.
    PlotMapIterator it = this->Internals->PlotMap.find(seriesName);
    if (it != this->Internals->PlotMap.end())
      {
      newMap[seriesName] = it->second;
      }
    else
      {
      PlotInfo& plotInfo = newMap[seriesName];
      plotInfo.Label = seriesName;
      vtkColor3ub color = this->Internals->Colors->GetColorRepeating(i);
      plotInfo.Color[0] = color.GetData()[0]/255.0;
      plotInfo.Color[1] = color.GetData()[1]/255.0;
      plotInfo.Color[2] = color.GetData()[2]/255.0;
      }
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

  this->RefreshPlots();

  bool skip = false;
  const char* propertyName = this->GetPropertyName(prop);
  vtkSmartPointer<vtkStringList> newValues = vtkSmartPointer<vtkStringList>::New();

  // Note: we could iterate over this->Internals->PlotMap, but just for
  // kicks we'll iterate over the table columns in order to respect the
  // column ordering, probably not needed... since we called RefreshPlots()
  // we know that PlotMap matches the table columns.
  int numberOfColumns = this->Internals->Table->GetNumberOfColumns();
  for (int i = 0; i < numberOfColumns; ++i)
    {
    const char* seriesName = this->Internals->Table->GetColumnName(i);
    if (!seriesName)
      {
      continue;
      }

    PlotInfo& plotInfo = this->Internals->PlotMap[seriesName];
    newValues->AddString(seriesName);

    if (strcmp(propertyName, "VisibilityInfo") == 0)
      {
      newValues->AddString(QString::number(plotInfo.Visible).toAscii().data());
      }
    else if (strcmp(propertyName, "LabelInfo") == 0)
      {
      newValues->AddString(plotInfo.Label);
      }
    else if (strcmp(propertyName, "LineThicknessInfo") == 0)
      {
      newValues->AddString(
        QString::number(plotInfo.LineThickness).toAscii().data());
      }
    else if (strcmp(propertyName, "ColorInfo") == 0)
      {
      newValues->AddString(QString::number(plotInfo.Color[0]).toAscii().data());
      newValues->AddString(QString::number(plotInfo.Color[1]).toAscii().data());
      newValues->AddString(QString::number(plotInfo.Color[2]).toAscii().data());
      }
    else if (strcmp(propertyName, "LineStyleInfo") == 0)
      {
      newValues->AddString(
        QString::number(plotInfo.LineStyle).toAscii().data());
      }
    else if (strcmp(propertyName, "MarkerStyleInfo") == 0)
      {
      newValues->AddString(
        QString::number(plotInfo.MarkerStyle).toAscii().data());
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

  this->RefreshLegendStatus();
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetTableVisibility(bool visible)
{
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
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
  plotInfo.Visible = visible;
  this->SetPlotVisibilityInternal(plotInfo, visible, name);
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLineThickness(const char* name,
                                                     int value)
{
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
  plotInfo.LineThickness = value;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLineStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
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
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
  plotInfo.Color[0] = r;
  plotInfo.Color[1] = g;
  plotInfo.Color[2] = b;
  if (plotInfo.Plot)
    {
    plotInfo.Plot->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetAxisCorner(const char*, int)
{

}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetMarkerStyle(const char* name, int style)
{
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
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
  PlotInfo& plotInfo = this->Internals->PlotMap[name];
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
