/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLineChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("Line") in the constructor.

#ifndef __vtkPVLineChartView_h
#define __vtkPVLineChartView_h

#include "vtkPVXYChartView.h"

class VTK_EXPORT vtkPVLineChartView : public vtkPVXYChartView
{
public:
  static vtkPVLineChartView* New();
  vtkTypeMacro(vtkPVLineChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVLineChartView();
  ~vtkPVLineChartView();

private:
  vtkPVLineChartView(const vtkPVLineChartView&); // Not implemented
  void operator=(const vtkPVLineChartView&); // Not implemented
//ETX
};

#endif
