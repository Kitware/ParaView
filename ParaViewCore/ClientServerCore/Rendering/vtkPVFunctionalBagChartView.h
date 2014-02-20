/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFunctionalBagChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFunctionalBagChartView
// .SECTION Description
// Subclass for vtkPVXYChartView that calls
// vtkPVXYChartView::SetChartType("FunctionalBag") in the constructor.

#ifndef __vtkPVFunctionalBagChartView_h
#define __vtkPVFunctionalBagChartView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVXYChartView.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVFunctionalBagChartView : public vtkPVXYChartView
{
public:
  static vtkPVFunctionalBagChartView* New();
  vtkTypeMacro(vtkPVFunctionalBagChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVFunctionalBagChartView();
  ~vtkPVFunctionalBagChartView();

private:
  vtkPVFunctionalBagChartView(const vtkPVFunctionalBagChartView&); // Not implemented
  void operator=(const vtkPVFunctionalBagChartView&); // Not implemented
//ETX
};

#endif
