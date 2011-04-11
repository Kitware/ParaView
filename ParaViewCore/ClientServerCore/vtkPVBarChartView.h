/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBarChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBarChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("Bar") in the constructor.

#ifndef __vtkPVBarChartView_h
#define __vtkPVBarChartView_h

#include "vtkPVXYChartView.h"

class VTK_EXPORT vtkPVBarChartView : public vtkPVXYChartView
{
public:
  static vtkPVBarChartView* New();
  vtkTypeMacro(vtkPVBarChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVBarChartView();
  ~vtkPVBarChartView();

private:
  vtkPVBarChartView(const vtkPVBarChartView&); // Not implemented
  void operator=(const vtkPVBarChartView&); // Not implemented
//ETX
};

#endif
