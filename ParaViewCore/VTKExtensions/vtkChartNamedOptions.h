/*=========================================================================

  Program:   ParaView
  Module:    vtkChartNamedOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkChartNamedOptions
// .SECTION Description
//

#ifndef __vtkChartNamedOptions_h
#define __vtkChartNamedOptions_h

#include "vtkObject.h"

class vtkChart;
class vtkTable;

class VTK_EXPORT vtkChartNamedOptions : public vtkObject
{
public:
  static vtkChartNamedOptions* New();
  vtkTypeMacro(vtkChartNamedOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set series visibility for the series with the given name.
  void SetVisibility(const char* name, int visible);

  // Description:
  // Get series visibility for the series with the given name.
  int GetVisibility(const char* name);

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
  // Class for storing individual series properties like label, visibility.
  class PlotInfo;

protected:
  vtkChartNamedOptions();
  ~vtkChartNamedOptions();

  // Description:
  // Initializes the plots map, and adds a default series to plot.
  void RefreshPlots();

  // Description:
  // If the plot exists this will set its visibility.  If the plot does not yet
  // exist and visible is true then the plot will be created.  The series name
  // is passed to this method so it can be used to initialize the vtkPlot if needed.
  void SetPlotVisibilityInternal(PlotInfo& info, bool visible,
                                 const char* seriesName);

  PlotInfo& GetPlotInfo(const char* seriesName);

private:
  vtkChartNamedOptions(const vtkChartNamedOptions&); // Not implemented
  void operator=(const vtkChartNamedOptions&); // Not implemented

  vtkTimeStamp RefreshTime;
  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
