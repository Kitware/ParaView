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
//----------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{
  this->Transform = vtkTransform::New();
  this->Transform->PostMultiply();
  this->Transform->Identity();
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
void vtkPrismView::UpdateWorldScale(const vtkBoundingBox& worldBounds)
{
    //now calculate out the new 4x4 matrix for the transform
    double matrix[16] =
      { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};

    const double* maxpoint = worldBounds.GetMaxPoint();
    matrix[0] = 100.0 / maxpoint[0];
    matrix[5] = 100.0 / maxpoint[1];
    matrix[10] = 100.0 / maxpoint[2];

    

    this->Transform->SetMatrix(matrix);
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
      ++numPrismBoundsFound;
      }
    }
  this->UpdateWorldScale(worldBounds);

  //now set the scale on each item
  double *scale = this->Transform->GetScale();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* info =
      this->ReplyInformationVector->GetInformationObject(cc);

    vtkDataRepresentation *repr = this->GetRepresentation(cc);
    vtkCompositeRepresentation *compositeRep = vtkCompositeRepresentation::SafeDownCast(repr);
    vtkCubeAxesRepresentation *cubeAxes = vtkCubeAxesRepresentation::SafeDownCast(repr);
    vtkSelectionRepresentation *selection = vtkSelectionRepresentation::SafeDownCast(repr);
    if(compositeRep)
      {
      vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(
        compositeRep->GetActiveRepresentation());
      if (prismRep)
        {
        prismRep->SetScale(scale[0],scale[1],scale[2]);
        }
      }
    else if (cubeAxes)
      {
      cubeAxes->SetScale(scale[0],scale[1],scale[2]);
      }
    else if (selection)
      {
      selection->SetScale(scale[0],scale[1],scale[2]);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
