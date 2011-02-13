/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPV2DRenderView.h"

#include "vtkObjectFactory.h"
#include "vtkLegendScaleActor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPV2DRenderView);
//----------------------------------------------------------------------------
vtkPV2DRenderView::vtkPV2DRenderView()
{
  this->LegendScaleActor = vtkLegendScaleActor::New();
  this->LegendScaleActor->SetLabelMode(1);
  this->LegendScaleActor->SetLegendVisibility(0);
  this->LegendScaleActor->SetCornerOffsetFactor(1.0);
  this->LegendScaleActor->SetVisibility(0);
  this->GetNonCompositedRenderer()->AddActor(this->LegendScaleActor);

  this->SetCenterAxesVisibility(false);
  this->SetOrientationAxesVisibility(false);
  this->SetOrientationAxesInteractivity(false);
  this->SetInteractionMode(INTERACTION_MODE_2D);
}

//----------------------------------------------------------------------------
vtkPV2DRenderView::~vtkPV2DRenderView()
{
  this->LegendScaleActor->Delete();
}

//----------------------------------------------------------------------------
void vtkPV2DRenderView::SetInteractionMode(int mode)
{
  if (mode == INTERACTION_MODE_3D)
    {
    mode = INTERACTION_MODE_2D;
    }
  this->Superclass::SetInteractionMode(mode);
}

//----------------------------------------------------------------------------
void vtkPV2DRenderView::SetAxisVisibility(bool val)
{
  this->LegendScaleActor->SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPV2DRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
