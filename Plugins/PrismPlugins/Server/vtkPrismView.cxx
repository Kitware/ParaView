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
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPrismView);
vtkInformationKeyRestrictedMacro(vtkPrismView, PRISM_GEOMETRY_BOUNDS, DoubleVector,6);
//----------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{

}

//----------------------------------------------------------------------------
vtkPrismView::~vtkPrismView()
{
}

//----------------------------------------------------------------------------
void vtkPrismView::GatherRepresentationInformation()
{
  this->Superclass::GatherRepresentationInformation();

  int num_reprs = this->ReplyInformationVector->GetNumberOfInformationObjects();
  vtkBoundingBox worldBounds;
  int numPrismBoundsFound = 0;
  

  //This is what we want to do:
  //go through all the reps with the prism bounds key and determine what the world space
  //apply this scaling to everything in the view. 
  //Note we have a work around where the simulation data is not adding its bounds
  //to the prism geom bounds, while it is being scaled by those bounds. This is intended!

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
      ++numPrismBoundsFound;
      }
    }

  if ( numPrismBoundsFound > 0 )
    {
    //now calculate out the scale of each object
    double scale[3];
    scale[0] = 100.0 / worldBounds.GetLength(0);
    scale[1] = 100.0 / worldBounds.GetLength(1);
    scale[2] = 100.0 / worldBounds.GetLength(2);

    //now set the scale and center on each item
    for (int cc=0; cc < num_reprs; cc++)
      {
      vtkInformation* info =
        this->ReplyInformationVector->GetInformationObject(cc);

      vtkDataRepresentation *repr = this->GetRepresentation(cc);
      vtkCompositeRepresentation *compositeRep = vtkCompositeRepresentation::SafeDownCast(repr);
      if(compositeRep)
        {
        vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(
          compositeRep->GetActiveRepresentation());
        if (prismRep)
          {
          prismRep->SetScale(scale[0],scale[1],scale[2]);

          //set the center
          double center[3];
          worldBounds.GetMinPoint(center[0], center[1], center[2]);
          prismRep->SetOrigin(center[0],center[1],center[2]);
          }
        }
      else
        {
        vtkCubeAxesRepresentation *cubeAxes = vtkCubeAxesRepresentation::SafeDownCast(repr);
        if ( cubeAxes )
          {
          //set the positon
          double pos[3];
          worldBounds.GetMinPoint(pos[0], pos[1], pos[2]);
          cubeAxes->SetPosition(pos[0],pos[1],pos[2]);

          cubeAxes->SetScale(scale[0],scale[1],scale[2]);
          }

        }
      }
    }  
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
