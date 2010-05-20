/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartViewProxy - abstract base class for all Chart Views.
// .SECTION Description
// vtkSMChartViewProxy is an abstract base class for all vtkQtChartView
// subclasses. These are the Qt-based charting views.

#ifndef __vtkSMChartViewProxy_h
#define __vtkSMChartViewProxy_h

#include "vtkSMViewProxy.h"

class vtkQtChartView;
class vtkQtChartWidget;

class VTK_EXPORT vtkSMChartViewProxy : public vtkSMViewProxy
{
public:
  vtkTypeMacro(vtkSMChartViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Saves the chart view as an image file.  See vtkQtChartView::SaveImage().
  // Returns true on success.
  bool WriteImage(const char* filename);

  //BTX
  // Description:
  // Provides access to the chart view's widget.
  vtkQtChartWidget* GetChartWidget();

  // Description:
  // Provides access to the vtk chart view.
  vtkQtChartView* GetChartView();
  //ETX

//BTX
protected:
  vtkSMChartViewProxy();
  ~vtkSMChartViewProxy();

  // Description:
  // Called once in CreateVTKObjects() to create a new chart view.
  virtual vtkQtChartView* NewChartView()=0;

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender().
  // Default implementation is empty.
  virtual void PerformRender();

  vtkQtChartView* ChartView;
private:
  vtkSMChartViewProxy(const vtkSMChartViewProxy&); // Not implemented
  void operator=(const vtkSMChartViewProxy&); // Not implemented
//ETX
};

#endif

