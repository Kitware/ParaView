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
#include "vtkPrismView.h"

#include "vtkBoundingBox.h"
#include "vtkCubeAxesRepresentation.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPrismRepresentation.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkSelectionRepresentation.h"
#include "vtkTransform.h"
#include "vtk3DWidgetRepresentation.h"

vtkStandardNewMacro(vtkPrismView);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_GEOMETRY_BOUNDS, DoubleVector,6);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_THRESHOLD_BOUNDS, DoubleVector,6);
//----------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{
  this->Transform = vtkTransform::New();
  this->Transform->PostMultiply();
  this->Transform->Identity();
  
  this->WorldScaleMode[0] =  this->WorldScaleMode[1] = this->WorldScaleMode[2] = 0;
  this->CustomWorldBounds[0] =  this->CustomWorldBounds[1] = this->CustomWorldBounds[2] = 0;
  this->CustomWorldBounds[3] =  this->CustomWorldBounds[4] = this->CustomWorldBounds[5] = 0;
  this->FullWorldBounds[0] =  this->FullWorldBounds[1] = this->FullWorldBounds[2] = 0;
  this->FullWorldBounds[3] =  this->FullWorldBounds[4] = this->FullWorldBounds[5] = 0;
  this->ThresholdWorldBounds[0] =  this->ThresholdWorldBounds[1] = this->ThresholdWorldBounds[2] = 0;
  this->ThresholdWorldBounds[3] =  this->ThresholdWorldBounds[4] = this->ThresholdWorldBounds[5] = 0;
}

//----------------------------------------------------------------------------
vtkPrismView::~vtkPrismView()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkPrismView::AddRepresentation(vtkDataRepresentation* rep)
{
  if (!this->IsRepresentationPresent(rep))
    {
    vtk3DWidgetRepresentation *widget = vtk3DWidgetRepresentation::SafeDownCast(rep);
    if ( widget )
      {
      widget->SetCustomWidgetTransform(this->Transform);
      }
    }
  this->Superclass::AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPrismView::RemoveRepresentation(vtkDataRepresentation* rep)
{
  if (this->IsRepresentationPresent(rep))
    {
    vtk3DWidgetRepresentation *widget = vtk3DWidgetRepresentation::SafeDownCast(rep);
    if ( widget )
      {
      widget->SetCustomWidgetTransform(NULL);
      }
    }
  this->Superclass::RemoveRepresentation(rep);
}

//----------------------------------------------------------------------------
bool vtkPrismView::UpdateWorldScale(const vtkBoundingBox& worldBounds,
  const vtkBoundingBox& thresholdBounds)
{
  //update the world and threshold ivars
  worldBounds.GetBounds(this->FullWorldBounds);
  thresholdBounds.GetBounds(this->ThresholdWorldBounds);

  //now calculate out the new 4x4 matrix for the transform
  double matrix[16] =
    {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

  double bounds[6];
  int index = 0;
  for ( int i=0; i < 3; ++i, index+=2)
    {      
    switch(this->WorldScaleMode[i])
      {
      case vtkPrismView::FullBounds:
        bounds[index] = worldBounds.GetBound(index);
        bounds[index+1] = worldBounds.GetBound(index+1);
        break;
      case vtkPrismView::ThresholdBounds:
        bounds[index] = thresholdBounds.GetBound(index);
        bounds[index+1] = thresholdBounds.GetBound(index+1);
        break;
      case vtkPrismView::CustomBounds:
        bounds[index] = this->CustomWorldBounds[index];
        bounds[index+1] = this->CustomWorldBounds[index+1];
        break;
      }
    }

  matrix[0] = 100.0 / (bounds[1] - bounds[0]);
  matrix[5] = 100.0 / (bounds[3] - bounds[2]);
  matrix[10] = 100.0 / (bounds[5] - bounds[4]);

  double* scale =this->Transform->GetScale();
  if (scale[0] != matrix[0] ||
      scale[1] != matrix[5] ||
      scale[2] != matrix[10])
      {
      this->Transform->SetMatrix(matrix);
      return true;
      }
  return false;
}
//----------------------------------------------------------------------------
void vtkPrismView::GatherRepresentationInformation()
{
  this->Superclass::GatherRepresentationInformation();

  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  vtkBoundingBox worldBounds, thresholdBounds;
  

  //This is what we want to do:
  //go through all the reps with the prism bounds key and determine what the world space
  //apply this scaling to everything in the view. Handle the cube axis as a special case
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);
    if (info->Has(vtkPrismView::PRISM_GEOMETRY_BOUNDS()))
      {
      //collect all the bounds of the world
      vtkBoundingBox repBounds;
      repBounds.AddBounds(info->Get(vtkPrismView::PRISM_GEOMETRY_BOUNDS()));
      worldBounds.AddBox(repBounds);

      //collect all the bounds of the thresholded world
      vtkBoundingBox tBounds;
      tBounds.AddBounds(info->Get(vtkPrismView::PRISM_THRESHOLD_BOUNDS()));
      thresholdBounds.AddBox(tBounds);
      }
    }
  bool updatedWorldScale = this->UpdateWorldScale(worldBounds, thresholdBounds);

  //now set the scale on each item
  double *scale = this->Transform->GetScale();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkDataRepresentation *repr = this->GetRepresentation(cc);
    vtkCompositeRepresentation *comp =
      vtkCompositeRepresentation::SafeDownCast(repr);
    if ( comp )
      {
      vtkPrismRepresentation *prismRep =
        vtkPrismRepresentation::SafeDownCast(comp->GetActiveRepresentation());
      if ( prismRep )
        {
        prismRep->SetScale(scale[0],scale[1],scale[2]);      
        continue;
        }
      }

    vtkCubeAxesRepresentation *cubeAxes =
      vtkCubeAxesRepresentation::SafeDownCast(repr);
    if (cubeAxes)
      {
      cubeAxes->SetScale(scale[0],scale[1],scale[2]);
      continue;
      }

    vtkSelectionRepresentation *selection =
      vtkSelectionRepresentation::SafeDownCast(repr);
    if (selection)
      {
      selection->SetScale(scale[0],scale[1],scale[2]);
      continue;
      }

    vtk3DWidgetRepresentation *widget = 
      vtk3DWidgetRepresentation::SafeDownCast(repr);
    if ( widget && updatedWorldScale )
      {
      //if the world scale has changed while the widget is active, remove and 
      //re add the transform to get the widget to be transformed properly
      widget->SetCustomWidgetTransform(NULL);
      widget->SetCustomWidgetTransform(this->Transform);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
