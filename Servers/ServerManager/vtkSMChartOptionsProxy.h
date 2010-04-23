/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartOptionsProxy - a helper proxy used by
// vtkSMBarChartViewProxy and vtkSMLineChartViewProxy to setup the chart
// options.
// .SECTION Description
// vtkSMChartOptionsProxy is a helper proxy used by
// vtkSMBarChartViewProxy and vtkSMLineChartViewProxy to setup the chart
// options. This proxy isn't a "proxy" in the true sense i.e. it does not
// represent a VTKObject on the server side.

#ifndef __vtkSMChartOptionsProxy_h
#define __vtkSMChartOptionsProxy_h

#include "vtkSMProxy.h"

class vtkQtChartView;
class vtkSMChartViewProxy;

class VTK_EXPORT vtkSMChartOptionsProxy : public vtkSMProxy
{
public:
  static vtkSMChartOptionsProxy* New();
  vtkTypeMacro(vtkSMChartOptionsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the chart's title.
  // Chart title can take a special
  // keyword ${TIME} which will be replaced with the view's time on the fly.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitle(const char* title);

  // Description:
  // Set the chart title's font.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleFont(const char* family, int pointSize, bool bold, bool italic);

  // Description:
  // Set the chart title's color.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleColor(double red, double green, double blue);

  // Description:
  // Set the chart title's alignment.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetTitleAlignment(int alignment);

  // Description:
  // Set the chart axis title for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitle(int index, const char* title);

  // Description:
  // Set the chart axis title's font for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitleFont(int index, const char* family, int pointSize,
    bool bold, bool italic);

  // Description:
  // Set the chart axis title's color for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitleColor(int index, double red, double green, double blue);

  // Description:
  // Set the chart axis title's alignment for the given index.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisTitleAlignment(int index, int alignment);

  // Description:
  // Sets whether or not the chart legend is visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetLegendVisibility(bool visible);

  // Description:
  // Sets the legend location.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetLegendLocation(int location);

  // Description:
  // Sets the legend flow.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetLegendFlow(int flow);

  // Description:
  // Sets whether or not the given axis is visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisVisibility(int index, bool visible);

  // Description:
  // Sets the color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisColor(int index, double red, double green, double blue);

  // Description:
  // Sets whether or not the grid for the given axis is visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridVisibility(int index, bool visible);

  // Description:
  // Sets the grid color type for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridColorType(int index, int gridColorType);

  // Description:
  // Sets the grid color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetGridColor(int index, double red, double green, double blue);

  // Description:
  // Sets whether or not the labels for the given axis are visible.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelVisibility(int index, bool visible);

  // Description:
  // Set the axis label font for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelFont(int index, const char* family, int pointSize,
    bool bold, bool italic);

  // Description:
  // Sets the axis label color for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelColor(int index, double red, double green, double blue);

  // Description:
  // Sets the axis label notation for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelNotation(int index, int notation);

  // Description:
  // Sets the axis label precision for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisLabelPrecision(int index, int precision);

  // Description:
  // Sets the scale for the given axis (Linear or Logarithmic).
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisScale(int index, int scale);

  // Description:
  // Sets the behavior for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisBehavior(int index, int behavior);

  // Description:
  // Sets the best fir range for the given axis.
  // These methods should not be called directly. They are made public only so
  // that the client-server-stream-interpreter can invoke them. Use the
  // corresponding properties to change these values.
  void SetAxisRange(int index, double minimum, double maximum);
 
//BTX
protected:
  vtkSMChartOptionsProxy();
  ~vtkSMChartOptionsProxy();

  friend class vtkSMChartViewProxy;

  // Description:
  void PrepareForRender(vtkSMChartViewProxy* viewProxy);

  // Description:
  // Get/Set the vtkQtChartView whose options are to be edited by this proxy.
  // This 
  vtkGetObjectMacro(ChartView, vtkQtChartView);
  void SetChartView(vtkQtChartView*);

  char* TitleInternal;
  vtkSetStringMacro(TitleInternal);
  
  double AxisRanges[4][2];
  int AxisBehavior[4];
  bool AxisRangesDirty;
  vtkQtChartView* ChartView;

  void UpdateAxisRanges();

private:
  vtkSMChartOptionsProxy(const vtkSMChartOptionsProxy&); // Not implemented
  void operator=(const vtkSMChartOptionsProxy&); // Not implemented
//ETX
};

#endif

