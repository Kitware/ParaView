/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotMatrixView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkPVPlotMatrixView_h
#define __vtkPVPlotMatrixView_h

#include "vtkPVContextView.h"

class vtkScatterPlotMatrix;

class VTK_EXPORT vtkPVPlotMatrixView : public vtkPVContextView
{
public:
  static vtkPVPlotMatrixView* New();
  vtkTypeMacro(vtkPVPlotMatrixView, vtkPVContextView);
  void PrintSelf(ostream &os, vtkIndent indent);

  vtkAbstractContextItem* GetContextItem();

  // Description:
  // Set the title of the active plot.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitle(const char* title);

  // Description:
  // Set the active plot title's font.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleFont(const char* family, int pointSize, bool bold, bool italic);

  // Description:
  // Set the active plot title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleColor(double red, double green, double blue);

  // Description:
  // Set the active plot title's alignment.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleAlignment(int alignment);

  // Description:
  // Set the gutter that should be left between the charts in the matrix.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  virtual void SetGutter(float x, float y);

  // Description:
  // Set/get the borders of the chart matrix (space in pixels around each chart).
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  virtual void SetBorders(int left, int bottom, int right, int top);

  // Description:
  // Sets whether or not the grid for the given axis is visible given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridVisibility(int plotType, bool visible);

  // Description:
  // Sets the background color for the chart given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetBackgroundColor(int plotType,
    double red, double green, double blue, double alpha=0.0);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets whether or not the labels for the axes are visible, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelVisibility(int plotType, bool visible);

  // Description:
  // Set the axis label font for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelFont(int plotType, const char* family, int pointSize, bool bold,
    bool italic);

  // Description:
  // Sets the axis label color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelColor(int plotType, double red, double green, double blue);

  // Description:
  // Sets the axis label notation for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelNotation(int plotType, int notation);

  // Description:
  // Sets the axis label precision for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelPrecision(int plotType, int precision);

  // Description:
  // Set chart's tooltip notation and precision, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);

protected:
  vtkPVPlotMatrixView();
  ~vtkPVPlotMatrixView();

  // Description:
  // The callback function when SelectionChangedEvent is invoked from
  // the Big chart in vtkScatterPlotMatrix.
  void PlotMatrixSelectionCallback(vtkObject*, unsigned long, void*);

private:
  vtkPVPlotMatrixView(const vtkPVPlotMatrixView&); // Not implemented.
  void operator=(const vtkPVPlotMatrixView&); // Not implemented.

  vtkScatterPlotMatrix *PlotMatrix;
};

#endif
