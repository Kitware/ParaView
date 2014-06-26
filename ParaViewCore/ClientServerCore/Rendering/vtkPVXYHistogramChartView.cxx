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
#include "vtkPVHistogramChartRepresentation.h"
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
vtkAbstractContextItem* vtkPVXYHistogramChartView::GetContextItem()
{
  return this->GetChart();
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
vtkSelection* vtkPVXYHistogramChartView::GetSelection()
{
  int numReprs = this->GetNumberOfRepresentations();
  for (int i = 0; i < numReprs; i++)
    {
    vtkPVHistogramChartRepresentation * repr =
      vtkPVHistogramChartRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (repr && repr->GetVisibility())
      {
      return repr->GetSelection();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::Render(bool interactive)
{
  this->Superclass::Render(interactive);
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::SelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::Update()
{
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVXYHistogramChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
