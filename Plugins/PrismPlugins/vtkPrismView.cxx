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

#include "vtk3DWidgetRepresentation.h"
#include "vtkBoundingBox.h"
#include "vtkCubeAxesRepresentation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPrismPrivate.h"
#include "vtkPrismRepresentation.h"
#include "vtkPVCompositeRepresentation.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkSelectionRepresentation.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"


vtkStandardNewMacro(vtkPrismView);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_GEOMETRY_BOUNDS, DoubleVector,6);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_THRESHOLD_BOUNDS, DoubleVector,6);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_USE_LOG_SCALING, IntegerVector,3);
vtkInformationKeyMacro(vtkPrismView, PRISM_TABLE_ID, Integer);

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

  this->LogScalingEnabled[0] = this->LogScalingEnabled[1] = this->LogScalingEnabled[2] = 0;
  this->PrismTableId = 0;
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
bool vtkPrismView::UpdateWorldScale()
{
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
        bounds[index] = this->FullWorldBounds[index];
        bounds[index+1] =this->FullWorldBounds[index+1];
        break;
      case vtkPrismView::ThresholdBounds:
        bounds[index] = this->ThresholdWorldBounds[index];
        bounds[index+1] = this->ThresholdWorldBounds[index+1];
        break;
      case vtkPrismView::CustomBounds:
        bounds[index] = this->CustomWorldBounds[index];
        bounds[index+1] = this->CustomWorldBounds[index+1];
        break;
      }
    }


  //now scale the bounds
  bool valid =
      vtkPrismCommon::scaleBounds(this->LogScalingEnabled,this->PrismTableId,bounds);
  if(!valid)
    {
    return false;
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
void vtkPrismView::Update()
{
  vtkTimerLog::MarkStartEvent("PrismView::Update");
  this->Superclass::Update();

  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  vtkBoundingBox worldBounds, thresholdBounds;
  bool has_reps  = false;
  
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
      repBounds.AddBounds(vtkPrismView::PRISM_GEOMETRY_BOUNDS()->Get(info));
      worldBounds.AddBox(repBounds);

      //collect all the bounds of the thresholded world
      vtkBoundingBox tBounds;
      tBounds.AddBounds(vtkPrismView::PRISM_THRESHOLD_BOUNDS()->Get(info));
      thresholdBounds.AddBox(tBounds);

      //we are going to presume that all the objects have the same scaling
      //modes and table ids as that is the only way sane way to determine
      //how to scale the bounds
      if(has_reps==false)
        {
        //do this only once
        vtkPrismView::PRISM_USE_LOG_SCALING()->Get(info,this->LogScalingEnabled);
        this->PrismTableId = vtkPrismView::PRISM_TABLE_ID()->Get(info);
        }
      has_reps = true;
      }
    }

  //update the world and threshold ivars
  if (has_reps)
    {
    worldBounds.GetBounds(this->FullWorldBounds);
    thresholdBounds.GetBounds(this->ThresholdWorldBounds);
    }
  else
    {
    vtkMath::UninitializeBounds(this->FullWorldBounds);
    vtkMath::UninitializeBounds(this->ThresholdWorldBounds);
    }

  this->SynchronizedWindows->SynchronizeBounds(this->FullWorldBounds);
  this->SynchronizedWindows->SynchronizeBounds(this->ThresholdWorldBounds);
  bool updatedWorldScale = this->UpdateWorldScale();

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
  vtkTimerLog::MarkEndEvent("PrismView::Update");
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
