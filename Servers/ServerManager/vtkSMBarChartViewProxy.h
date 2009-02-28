/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBarChartViewProxy
// .SECTION Description
//

#ifndef __vtkSMBarChartViewProxy_h
#define __vtkSMBarChartViewProxy_h

#include "vtkSMViewProxy.h"

class vtkQtChartWidget;
class vtkQtBarChartView;

class VTK_EXPORT vtkSMBarChartViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMBarChartViewProxy* New();
  vtkTypeRevisionMacro(vtkSMBarChartViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Provides access to the bar chart widget.
  vtkQtChartWidget* GetChartWidget();
  //ETX

  // Description:
  // Provides access to the bar chart view.
  vtkQtBarChartView* GetBarChartView();

  // Description:
  // Sets the bar chart help format.
  // Don't call these methods directly, the should be used via properties alone.
  void SetHelpFormat(const char* format);

  // Description:
  // Sets the bar outline style.
  // Don't call these methods directly, the should be used via properties alone.
  void SetOutlineStyle(int outline);

  // Description:
  // Sets the bar group width fraction.
  // Don't call these methods directly, the should be used via properties alone.
  void SetBarGroupFraction(float fraction);

  // Description:
  // Sets the bar width fraction.
  // Don't call these methods directly, the should be used via properties alone.
  void SetBarWidthFraction(float fraction);

//BTX
protected:
  vtkSMBarChartViewProxy();
  ~vtkSMBarChartViewProxy();

  // Overridden to set up the ChartOptionsProxy.
  virtual void CreateVTKObjects();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender(). 
  // Default implementation is empty.
  virtual void PerformRender();


  vtkQtBarChartView* ChartView;

private:
  vtkSMBarChartViewProxy(const vtkSMBarChartViewProxy&); // Not implemented
  void operator=(const vtkSMBarChartViewProxy&); // Not implemented
//ETX
};

#endif

