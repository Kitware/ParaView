/*=========================================================================

  Program:   ParaView
  Module:    vtkPVParallelCoordinatesRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVParallelCoordinatesRepresentation.h"

#include "vtkChartParallelCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPVContextView.h"
#include "vtkTable.h"
#include "vtkStdString.h"

vtkStandardNewMacro(vtkPVParallelCoordinatesRepresentation);
//----------------------------------------------------------------------------
vtkPVParallelCoordinatesRepresentation::vtkPVParallelCoordinatesRepresentation()
{
}

//----------------------------------------------------------------------------
vtkPVParallelCoordinatesRepresentation::~vtkPVParallelCoordinatesRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVParallelCoordinatesRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  if (this->GetChart())
    {
    // Set the table, in case it has changed.
    this->GetChart()->GetPlot(0)->SetInput(this->GetLocalOutput());
    this->GetChart()->SetVisible(this->GetVisibility());
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVParallelCoordinatesRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->SetInput(0);
    this->GetChart()->SetVisible(false);
    }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
vtkChartParallelCoordinates* vtkPVParallelCoordinatesRepresentation::GetChart()
{
  if (this->ContextView)
    {
    return vtkChartParallelCoordinates::SafeDownCast(
      this->ContextView->GetChart());
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  if (this->GetChart())
    {
    this->GetChart()->SetVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetSeriesVisibility(
    const char* name, int visible)
{
  if (this->GetChart())
    {
    this->GetChart()->SetColumnVisibility(name, visible != 0);
    }
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetLabel(const char*,
                                                           const char*)
{

}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetLineThickness(int value)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetLineStyle(int value)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetLineType(value);
    }
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetColor(double r, double g,
                                                           double b)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetColorF(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkPVParallelCoordinatesRepresentation::SetOpacity(double opacity)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetOpacityF(opacity);
    }
}

//----------------------------------------------------------------------------
int vtkPVParallelCoordinatesRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }

  if (this->GetChart())
    {
    //vtkSelection *sel =
    //  vtkSelection::SafeDownCast(this->SelectionRepresentation->GetOutput());
    //this->AnnLink->SetCurrentSelection(sel);
    //this->GetChart()->SetAnnotationLink(AnnLink);

    // Set the table, in case it has changed.
    this->GetChart()->GetPlot(0)->SetInput(this->GetLocalOutput());
    }

  return 1;
}
