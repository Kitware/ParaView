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
// .NAME vtkSMBarChartViewProxy - view proxy for vtkQtBarChartView
// .SECTION Description
// vtkSMBarChartViewProxy is a concrete subclass of vtkSMChartViewProxy that
// creates a vtkQtBarChartView as the chart view.

#ifndef __vtkSMBarChartViewProxy_h
#define __vtkSMBarChartViewProxy_h

#include "vtkSMChartViewProxy.h"

class vtkQtBarChartView;

class VTK_EXPORT vtkSMBarChartViewProxy : public vtkSMChartViewProxy
{
public:
  static vtkSMBarChartViewProxy* New();
  vtkTypeMacro(vtkSMBarChartViewProxy, vtkSMChartViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the bar chart view.
//BTX
  vtkQtBarChartView* GetBarChartView();
//ETX

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

  // Description:
  // Called once in CreateVTKObjects() to create a new chart view.
  virtual vtkQtChartView* NewChartView();

private:
  vtkSMBarChartViewProxy(const vtkSMBarChartViewProxy&); // Not implemented
  void operator=(const vtkSMBarChartViewProxy&); // Not implemented
//ETX
};

#endif

