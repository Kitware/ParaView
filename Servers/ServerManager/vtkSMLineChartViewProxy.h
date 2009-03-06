/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineChartViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLineChartViewProxy
// .SECTION Description
//

#ifndef __vtkSMLineChartViewProxy_h
#define __vtkSMLineChartViewProxy_h

#include "vtkSMViewProxy.h"

class vtkQtChartWidget;
class vtkQtLineChartView;

class VTK_EXPORT vtkSMLineChartViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMLineChartViewProxy* New();
  vtkTypeRevisionMacro(vtkSMLineChartViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Provides access to the bar chart widget.
  vtkQtChartWidget* GetChartWidget();
  //ETX

  // Description:
  // Provides access to the line chart view.
  vtkQtLineChartView* GetLineChartView();

  // Description:
  // Sets the line chart help format.
  // Don't call these methods directly, the should be used via properties alone.
  void SetHelpFormat(const char* format);

//BTX
protected:
  vtkSMLineChartViewProxy();
  ~vtkSMLineChartViewProxy();

  // Description:
  virtual void CreateVTKObjects();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender(). 
  // Default implementation is empty.
  virtual void PerformRender();


  vtkQtLineChartView* ChartView;

private:
  vtkSMLineChartViewProxy(const vtkSMLineChartViewProxy&); // Not implemented
  void operator=(const vtkSMLineChartViewProxy&); // Not implemented
//ETX
};

#endif

