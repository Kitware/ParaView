/*=========================================================================

  Program:   ParaView
  Module:    vtkContextNamedOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContextNamedOptions
// .SECTION Description
//

#ifndef __vtkContextNamedOptions_h
#define __vtkContextNamedOptions_h

#include "vtkObject.h"

class vtkChart;
class vtkTable;

class VTK_EXPORT vtkContextNamedOptions : public vtkObject
{
public:
  static vtkContextNamedOptions* New();
  vtkTypeMacro(vtkContextNamedOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set series visibility for the series with the given name.
  void SetVisibility(const char* name, int visible);
  void SetLineThickness(const char* name, int value);
  void SetLineStyle(const char* name, int value);
  void SetColor(const char* name, double r, double g, double b);
  void SetAxisCorner(const char* name, int corner);
  void SetMarkerStyle(const char* name, int style);
  void SetLabel(const char* name, const char* label);

  int GetVisibility(const char* name);
  int GetLineThickness(const char* name);
  const char* GetLabel(const char* name);
  int GetLineStyle(const char* name);
  int GetMarkerStyle(const char* name);
  int GetAxisCorner(const char* name);
  void GetColor(const char* name, double rgb[3]);

  // Description:
  // Set the X series to be used for the plots, if NULL then the index of the
  // y series should be used.
  void SetXSeriesName(const char* name);
  const char* GetXSeriesName();

  // Description:
  // Set whether the index should be used for the x axis.
  void SetUseIndexForXAxis(bool useIndex);

  // Description:
  // Hides or plots that belong to this table.  When showing,
  // only plots that are actually marked visible will be shown.
  void SetTableVisibility(bool visible);

  // Description:
  // Set the type of plots that will be added to charts by this proxy.
  // Uses the enum from vtkChart.
  void SetChartType(int type);

  // Description:
  // Get the type of plots that will be added to charts by this proxy.
  // Uses the enum from vtkChart.
  int GetChartType();

// This BTX is here because currently vtkCharts is not client-server wrapped.
//BTX
  // Description:
  // Sets the internal chart object whose options will be manipulated.
  void SetChart(vtkChart* chart);
  vtkChart * GetChart();
//ETX


  // Description:
  // Sets the internal table object that can be plotted.
  void SetTable(vtkTable* table);
  vtkTable* GetTable();

  void RemovePlotsFromChart();

//BTX
  // Description:
  // Class for storing individual series properties like color, label, line thickness...
  class PlotInfo;
//ETX

//BTX
protected:
  vtkContextNamedOptions();
  ~vtkContextNamedOptions();

  // Description:
  // Initializes the plots map, and adds a default series to plot
  void RefreshPlots();

  // Description:
  // If the plot exists this will set its visibility.  If the plot does not yet
  // exist and visible is true then the plot will be created.  The series name
  // is passed to this method so it can be used to initialize the vtkPlot if needed.
  void SetPlotVisibilityInternal(PlotInfo& info, bool visible, const char* seriesName);

  PlotInfo& GetPlotInfo(const char* seriesName);

private:
  vtkContextNamedOptions(const vtkContextNamedOptions&); // Not implemented
  void operator=(const vtkContextNamedOptions&); // Not implemented

  vtkTimeStamp RefreshTime;
  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
