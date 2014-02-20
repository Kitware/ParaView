/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBagChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("Bag") in the constructor.

#ifndef __vtkPVBagChartView_h
#define __vtkPVBagChartView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVXYChartView.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVBagChartView : public vtkPVXYChartView
{
public:
  static vtkPVBagChartView* New();
  vtkTypeMacro(vtkPVBagChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVBagChartView();
  ~vtkPVBagChartView();

private:
  vtkPVBagChartView(const vtkPVBagChartView&); // Not implemented
  void operator=(const vtkPVBagChartView&); // Not implemented
//ETX
};

#endif
