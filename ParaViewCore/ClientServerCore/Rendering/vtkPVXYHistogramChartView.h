/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYHistogramChartView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXYHistogramChartView - vtkPVView subclass for drawing charts
// .SECTION Description

#ifndef __vtkPVXYHistogramChartView_h
#define __vtkPVXYHistogramChartView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVXYChartView.h"
#include "vtkAxis.h" //for enums.

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVXYHistogramChartView :
  public vtkPVXYChartView
{
public:
  static vtkPVXYHistogramChartView* New();
  vtkTypeMacro(vtkPVXYHistogramChartView, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Representations can use this method to set the selection for a particular
  // representation. Subclasses override this method to pass on the selection to
  // the chart using annotation link. Note this is meant to pass selection for
  // the local process alone. The view does not manage data movement for the
  // selection.
  virtual void SetSelection(
    vtkChartRepresentation* repr, vtkSelection* selection);

//BTX
protected:
  vtkPVXYHistogramChartView();
  ~vtkPVXYHistogramChartView();

private:
  vtkPVXYHistogramChartView(const vtkPVXYHistogramChartView&); // Not implemented
  void operator=(const vtkPVXYHistogramChartView&); // Not implemented
//ETX
};

#endif
