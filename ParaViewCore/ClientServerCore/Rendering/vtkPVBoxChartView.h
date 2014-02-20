/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBoxChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("Box") in the constructor.

#ifndef __vtkPVBoxChartView_h
#define __vtkPVBoxChartView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVXYChartView.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVBoxChartView : public vtkPVXYChartView
{
public:
  static vtkPVBoxChartView* New();
  vtkTypeMacro(vtkPVBoxChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVBoxChartView();
  ~vtkPVBoxChartView();

private:
  vtkPVBoxChartView(const vtkPVBoxChartView&); // Not implemented
  void operator=(const vtkPVBoxChartView&); // Not implemented
//ETX
};

#endif
