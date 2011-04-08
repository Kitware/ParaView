/*=========================================================================

  Program:   ParaView
  Module:    vtkPVParallelCoordinatesChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVParallelCoordinatesChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("ParallelCoordinates") in the constructor.

#ifndef __vtkPVParallelCoordinatesChartView_h
#define __vtkPVParallelCoordinatesChartView_h

#include "vtkPVXYChartView.h"

class VTK_EXPORT vtkPVParallelCoordinatesChartView : public vtkPVXYChartView
{
public:
  static vtkPVParallelCoordinatesChartView* New();
  vtkTypeMacro(vtkPVParallelCoordinatesChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVParallelCoordinatesChartView();
  ~vtkPVParallelCoordinatesChartView();

private:
  vtkPVParallelCoordinatesChartView(const vtkPVParallelCoordinatesChartView&); // Not implemented
  void operator=(const vtkPVParallelCoordinatesChartView&); // Not implemented
//ETX
};

#endif
