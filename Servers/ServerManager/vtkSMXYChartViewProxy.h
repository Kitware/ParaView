/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYChartViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXYChartViewProxy - view proxy for vtkQtBarChartView
// .SECTION Description
// vtkSMXYChartViewProxy is a concrete subclass of vtkSMChartViewProxy that
// creates a vtkQtBarChartView as the chart view.

#ifndef __vtkSMXYChartViewProxy_h
#define __vtkSMXYChartViewProxy_h

#include "vtkSMContextViewProxy.h"

class vtkChartView;
class vtkChartXY;

class VTK_EXPORT vtkSMXYChartViewProxy : public vtkSMContextViewProxy
{
public:
  static vtkSMXYChartViewProxy* New();
  vtkTypeRevisionMacro(vtkSMXYChartViewProxy, vtkSMContextViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the chart type, defaults to line chart
  void SetChartType(const char *type);

  // Description:
  // Provides access to the bar chart view.
//BTX
  vtkChartXY* GetChartXY();
//ETX

//BTX
protected:
  vtkSMXYChartViewProxy();
  ~vtkSMXYChartViewProxy();

  // Description:
  // Called once in CreateVTKObjects() to create a new chart view.
  virtual vtkContextView* NewChartView();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender().
  // Default implementation is empty.
  virtual void PerformRender();

  // Description:
  // Pointer to the proxy's chart instance.
  vtkChartXY* Chart;

private:
  vtkSMXYChartViewProxy(const vtkSMXYChartViewProxy&); // Not implemented
  void operator=(const vtkSMXYChartViewProxy&); // Not implemented
//ETX
};

#endif
