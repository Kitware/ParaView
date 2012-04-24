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
#include "vtkClientServerStream.h" // For CS stream methods.

class vtkAbstractContextItem;

class VTK_EXPORT vtkSMPlotMatrixViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMPlotMatrixViewProxy* New();
  vtkTypeMacro(vtkSMPlotMatrixViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the gutter that should be left between the charts in the matrix.
  void SetGutter(float x, float y);
  void GetGutter(float* xy);
  
  // Description:
  // Set/get the borders of the chart matrix (space in pixels around each chart).
  void SetBorders(int left, int bottom, int right, int top);
  void GetBorders(int* data);

  // Description:
  // Set the scatter plot title's font.
  void SetScatterPlotTitleFont(const char* family, int pointSize, bool bold, bool italic);
  const char* GetScatterPlotTitleFontFamily();
  int GetScatterPlotTitleFontSize();
  bool GetScatterPlotTitleFontBold();
  bool GetScatterPlotTitleFontItalic();

  // Description:
  // Set the scatter plot title's color.
  void SetScatterPlotTitleColor(double red, double green, double blue);
  void GetScatterPlotTitleColor(double* rgba);

  // Description:
  // Set the scatter plot title.
  void SetScatterPlotTitle(const char* title);
  const char* GetScatterPlotTitle();

  // Description:
  // Set the scatter plot's title alignment.
  void SetScatterPlotTitleAlignment(int alignment);
  int GetScatterPlotTitleAlignment();

  // Description:
  // Sets whether or not the grid for the given axis is visible given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridVisibility(int plotType, bool visible);
  bool GetGridVisibility(int plotType);

  // Description:
  // Sets the background color for the chart given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetBackgroundColor(int plotType,
    double red, double green, double blue, double alpha=0.0);
  void GetBackgroundColor(int plotType, double* rgba);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisColor(int plotType, double red, double green, double blue);
  void GetAxisColor(int plotType, double* rgba);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridColor(int plotType, double red, double green, double blue);
  void GetGridColor(int plotType, double* rgba);

  // Description:
  // Sets whether or not the labels for the axes are visible, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelVisibility(int plotType, bool visible);
  bool GetAxisLabelVisibility(int plotType);

  // Description:
  // Set the axis label font for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelFont(int plotType, const char* family, int pointSize, bool bold,
    bool italic);
  const char* GetAxisLabelFontFamily(int plotType);
  int GetAxisLabelFontSize(int plotType);
  bool GetAxisLabelFontBold(int plotType);
  bool GetAxisLabelFontItalic(int plotType);

  // Description:
  // Sets the axis label color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelColor(int plotType, double red, double green, double blue);
  void GetAxisLabelColor(int plotType, double* rgba);

  // Description:
  // Sets the axis label notation for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelNotation(int plotType, int notation);
  int GetAxisLabelNotation(int plotType);

  // Description:
  // Sets the axis label precision for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelPrecision(int plotType, int precision);
  int GetAxisLabelPrecision(int plotType);

  // Description:
  // Set chart's tooltip notation and precision, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);
  int GetTooltipNotation(int plotType);
  int GetTooltipPrecision(int plotType);

  // Description:
  // Set the scatter plot selected row/column charts' background color.
  void SetScatterPlotSelectedRowColumnColor(double red, double green, double blue, double alpha);
  void GetScatterPlotSelectedRowColumnColor(double* rgba);

  // Description:
  // Set the scatter plot selected active chart background color.
  void SetScatterPlotSelectedActiveColor(double red, double green, double blue, double alpha);
  void GetScatterPlotSelectedActiveColor(double* rgba);

  // Description:
  // Update all the settings
  void UpdateSettings();

  // Description:
  // Provides access to the vtk plot matrix.
  virtual vtkAbstractContextItem* GetContextItem();
//BTX
protected:
  virtual void CreateVTKObjects();
  void ActivePlotChanged();

  void PostRender(bool);

  bool ActiveChanged;

  vtkSMPlotMatrixViewProxy();
  ~vtkSMPlotMatrixViewProxy();
  void SendAnimationPath();
  void AnimationTickEvent();
  void SendDouble3Vector(const char *func, 
                        int plotType, double *data);
  void SendDouble4Vector(const char *func, 
    int plotType, double *data);

  void SendIntValue(const char *func, 
    int plotType, int val);
  int ReceiveIntValue(const char *func, int charType);
  void ReceiveTypeDouble4Vector(const char *func, 
                        int plotType, double *data);
  void ReceiveDouble4Vector(const char *func, 
    double *data);
  
  const vtkClientServerStream& InvokeServerMethod(const char* method);
  const vtkClientServerStream& InvokeTypeServerMethod(const char* method, int chartType);

private:
  vtkSMPlotMatrixViewProxy(const vtkSMPlotMatrixViewProxy&); // Not implemented
  void operator=(const vtkSMPlotMatrixViewProxy&); // Not implemented
//ETX
};

#endif
