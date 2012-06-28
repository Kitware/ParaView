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
#include "vtkWeakPointer.h" // For ivars

class vtkChart;
class vtkTable;
class vtkScatterPlotMatrix;

class VTK_EXPORT vtkChartNamedOptions : public vtkObject
{
public:
  static vtkChartNamedOptions* New();
  vtkTypeMacro(vtkChartNamedOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set series visibility for the series with the given name.
  virtual void SetVisibility(const char* name, int visible);

  // Description:
  // Get series visibility for the series with the given name.
  virtual int GetVisibility(const char* name);

  // Description:
  // Set the label for the specified series.
  virtual void SetLabel(const char* name, const char* label);

  // Description:
  // Get the label for the specified series.
  virtual const char* GetLabel(const char* name);

  // Description:
  // Hides or plots that belong to this table.  When showing,
  // only plots that are actually marked visible will be shown.
  virtual void SetTableVisibility(bool visible);

  // Description:
  // Set the type of plots that will be added to charts by this proxy.
  // Uses the enum from vtkChart.
  virtual void SetChartType(int type);

  // Description:
  // Get the type of plots that will be added to charts by this proxy.
  // Uses the enum from vtkChart.
  virtual int GetChartType();

// This BTX is here because currently vtkCharts is not client-server wrapped.
//BTX
  // Description:
  // Sets the internal chart object whose options will be manipulated.
  void SetChart(vtkChart* chart);
  vtkChart * GetChart();

  // Description:
  // Sets the internal plot matrix object whose options will be manipulated.
  void SetPlotMatrix(vtkScatterPlotMatrix* plotmatrix);
  vtkScatterPlotMatrix * GetPlotMatrix();

//ETX

  // Description:
  // Update the plot options to ensure that the charts have are in sync.
  void UpdatePlotOptions();

  // Description:
  // Sets the internal table object that can be plotted.
  void SetTable(vtkTable* table);
  vtkTable* GetTable();

  virtual void RemovePlotsFromChart();

//BTX
  // Description:
  // Class for storing individual series properties like label, visibility.
  class PlotInfo;

protected:
  vtkChartNamedOptions();
  ~vtkChartNamedOptions();

  // Description:
  // Initializes the plots map, and adds a default series to plot.
  virtual void RefreshPlots();

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;
  vtkWeakPointer<vtkScatterPlotMatrix> PlotMatrix;
  int ChartType;
  bool TableVisibility;

private:
  vtkChartNamedOptions(const vtkChartNamedOptions&); // Not implemented
  void operator=(const vtkChartNamedOptions&); // Not implemented

  PlotInfo& GetPlotInfo(const char* seriesName);

  // Description:
  // If the plot exists this will set its visibility.  If the plot does not yet
  // exist and visible is true then the plot will be created.  The series name
  // is passed to this method so it can be used to initialize the vtkPlot if needed.
  void SetPlotVisibilityInternal(PlotInfo& info, bool visible,
                                 const char* seriesName);

  vtkTimeStamp RefreshTime;
  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
