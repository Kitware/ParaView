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
  // Get/set the active plot in the scatter plot matrix.
  void SetActivePlot(int i, int j);
  int GetActiveRow();
  int GetActiveColumn();

  // Description:
  // Clear the animation path, ensuring it is empty.
  void ClearAnimationPath();

  // Description:
  // Append to the animation path of the scatter plot matrix.
  void AddAnimationPath(int i, int j);

  // Description:
  // Append to the animation path of the scatter plot matrix.
  void StartAnimationPath();

  // Description:
  // Push the animation forward a frame.
  void AdvanceAnimationPath();

  // Description:
  // Set the title of the active plot.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitle(const char* title);
  const char* GetScatterPlotTitle();

  // Description:
  // Set the active plot title's font.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleFont(const char* family, int pointSize, bool bold, bool italic);
  const char* GetScatterPlotTitleFontFamily();
  int GetScatterPlotTitleFontSize();
  int GetScatterPlotTitleFontBold();
  int GetScatterPlotTitleFontItalic();

  // Description:
  // Set the active plot title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleColor(double red, double green, double blue);
  double* GetScatterPlotTitleColor();

  // Description:
  // Set the active plot title's alignment.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotTitleAlignment(int alignment);
  int GetScatterPlotTitleAlignment();

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
  int GetGridVisibility(int plotType);

  // Description:
  // Sets the background color for the chart given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetBackgroundColor(int plotType,
    double red, double green, double blue, double alpha=0.0);
  double* GetBackgroundColor(int plotType);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisColor(int plotType, double red, double green, double blue);
  double* GetAxisColor(int plotType);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridColor(int plotType, double red, double green, double blue);
  double* GetGridColor(int plotType);

  // Description:
  // Sets whether or not the labels for the axes are visible, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelVisibility(int plotType, bool visible);
  int GetAxisLabelVisibility(int plotType);

  // Description:
  // Set the axis label font for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelFont(int plotType, const char* family, int pointSize, bool bold,
    bool italic);
  const char* GetAxisLabelFontFamily(int plotType);
  int GetAxisLabelFontSize(int plotType);
  int GetAxisLabelFontBold(int plotType);
  int GetAxisLabelFontItalic(int plotType);

  // Description:
  // Sets the axis label color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelColor(int plotType, double red, double green, double blue);
  double* GetAxisLabelColor(int plotType);

  // Description:
  // Sets the axis label notation for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelNotation(int plotType, int notation);
  int GetAxisLabelNotation(int plotType);

  // Description:
  // Sets the axis label precision for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelPrecision(int plotType, int precision);
  int GetAxisLabelPrecision(int plotType);

  // Description:
  // Set chart's tooltip notation and precision, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);
  int GetTooltipNotation(int plotType);
  int GetTooltipPrecision(int plotType);

  // Description:
  // Set the scatter plot title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotSelectedRowColumnColor(double red, double green, double blue, double alpha);
  double* GetScatterPlotSelectedRowColumnColor();

  // Description:
  // Set the scatter plot title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetScatterPlotSelectedActiveColor(double red, double green, double blue, double alpha);
  double* GetScatterPlotSelectedActiveColor();

  // Description:
  // Update all the settings
  void UpdateSettings();

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
