/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYHistogramChartView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXYHistogramChartView.h"

#include "vtkAnnotationLink.h"
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkSelection.h"
#include "vtkStringArray.h"

#include <string>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPVXYHistogramChartView);

//----------------------------------------------------------------------------
vtkPVXYHistogramChartView::vtkPVXYHistogramChartView()
{
  this->SetChartType("Bar");
}

//----------------------------------------------------------------------------
vtkPVXYHistogramChartView::~vtkPVXYHistogramChartView()
{
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::SetSelection(
  vtkChartRepresentation* repr, vtkSelection* selection)
{
  (void)repr;
  // Only support for reset selection
  if (this->Chart && selection && selection->GetNumberOfNodes() == 0)
    {
    // we don't support multiple selection for now.
    this->Chart->GetAnnotationLink()->SetCurrentSelection(selection);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
