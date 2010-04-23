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
// .NAME vtkSMLineChartViewProxy - view proxy for vtkQtLineChartView
// .SECTION Description
// vtkSMLineChartViewProxy is a concrete subclass of vtkSMChartViewProxy that
// creates a vtkQtLineChartView as the chart view.

#ifndef __vtkSMLineChartViewProxy_h
#define __vtkSMLineChartViewProxy_h

#include "vtkSMChartViewProxy.h"

class vtkQtLineChartView;

class VTK_EXPORT vtkSMLineChartViewProxy : public vtkSMChartViewProxy
{
public:
  static vtkSMLineChartViewProxy* New();
  vtkTypeMacro(vtkSMLineChartViewProxy, vtkSMChartViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the line chart view.
//BTX
  vtkQtLineChartView* GetLineChartView();
//ETX

  // Description:
  // Sets the bar chart help format.
  // Don't call these methods directly, the should be used via properties alone.
  void SetHelpFormat(const char* format);

//BTX
protected:
  vtkSMLineChartViewProxy();
  ~vtkSMLineChartViewProxy();

  // Description:
  // Called once in CreateVTKObjects() to create a new chart view.
  virtual vtkQtChartView* NewChartView();

private:
  vtkSMLineChartViewProxy(const vtkSMLineChartViewProxy&); // Not implemented
  void operator=(const vtkSMLineChartViewProxy&); // Not implemented
//ETX
};

#endif

