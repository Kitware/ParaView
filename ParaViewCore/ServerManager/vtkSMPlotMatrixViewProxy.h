/*=========================================================================

Program:   ParaView
Module:    vtkSMContextViewProxy.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPlotMatrixViewProxy - Proxy class for plot matrix view

#ifndef __vtkSMPlotMatrixViewProxy_h
#define __vtkSMPlotMatrixViewProxy_h

#include "vtkSMContextViewProxy.h"

class VTK_EXPORT vtkSMPlotMatrixViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMPlotMatrixViewProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the title of the active plot.
  void SetScatterPlotTitle(const char* title);

  // Description:
  // Set the active plot title's font.
  void SetScatterPlotTitleFont(const char* family, int pointSize, bool bold, bool italic);

  // Description:
  // Set the active plot title's color.
  void SetScatterPlotTitleColor(double red, double green, double blue);

  // Description:
  // Set the chart title's alignment given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetScatterPlotTitleAlignment(int alignment);

  // Description:
  // Sets whether or not the grid for the given axis is visible given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridVisibility(int plotType, bool visible);

  // Description:
  // Sets the background color for the chart given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetBackgroundColor(int plotType, double red, double green, double blue, double alpha=0.0);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets whether or not the labels for the axes are visible, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelVisibility(int plotType, bool visible);

  // Description:
  // Set the axis label font for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelFont(int plotType, const char* family, int pointSize, bool bold,
    bool italic);

  // Description:
  // Sets the axis label color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets the axis label notation for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelNotation(int plotType, int notation);

  // Description:
  // Sets the axis label precision for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelPrecision(int plotType, int precision);

  // Description:
  // Set chart's tooltip notation and precision, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);

  // Description:
  // Set the gutter that should be left between the charts in the matrix.
  virtual void SetGutter(float x, float y);

  // Description:
  // Set/get the borders of the chart matrix (space in pixels around each chart).
  virtual void SetBorders(int left, int bottom, int right, int top);

  // Description:
  // Set the scatter plot title's color.
  void SetScatterPlotSelectedRowColumnColor(double red, double green, double blue, double alpha);

  // Description:
  // Set the scatter plot title's color.
  void SetScatterPlotSelectedActiveColor(double red, double green, double blue, double alpha);

  // Description:
  // Update all the settings
  void UpdateSettings();

//BTX
protected:
  vtkSMPlotMatrixViewProxy();
  ~vtkSMPlotMatrixViewProxy();
  void SendDouble3Vector(const char *func, 
                        int plotType, double *data);
  void SendDouble4Vector(const char *func, 
    int plotType, double *data);

  void SendIntValue(const char *func, 
    int plotType, int val);
  void ReceiveDouble3Vector(const char *func, 
                        int plotType, double *data);

private:
  vtkSMPlotMatrixViewProxy(const vtkSMPlotMatrixViewProxy&); // Not implemented
  void operator=(const vtkSMPlotMatrixViewProxy&); // Not implemented
//ETX
};

#endif
