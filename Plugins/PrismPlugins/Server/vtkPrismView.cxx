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
    scale[0] = 100 / worldBounds.GetLength(0);
    scale[1] = 100 / worldBounds.GetLength(1);
    scale[2] = 100 / worldBounds.GetLength(2);

    //now set the scale and center on each item
    for (int cc=0; cc < num_reprs; cc++)
      {
      vtkInformation* info =
        this->ReplyInformationVector->GetInformationObject(cc);
      if (info->Has(vtkPrismView::PRISM_GEOMETRY_BOUNDS()))
        {
        vtkDataRepresentation *repr = this->GetRepresentation(cc);
        vtkCompositeRepresentation *compositeRep =
          vtkCompositeRepresentation::SafeDownCast(repr);
        if(compositeRep)
          {
          vtkGeometryRepresentation *geomRep = vtkGeometryRepresentation::SafeDownCast(
            compositeRep->GetActiveRepresentation());
          if (geomRep)
            {
            geomRep->SetScale(scale[0],scale[1],scale[2]);

            //set the center
            double center[3];
            worldBounds.GetMinPoint(center[0], center[1], center[2]);
            geomRep->SetOrigin(center[0],center[1],center[2]);
            }
          }
        }
      }
    }
  this->Superclass::GatherRepresentationInformation();
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
