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
#include "vtkPrismCubeAxesRepresentation.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkPrismCubeAxesActor.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkCubeAxesActor.h"

vtkStandardNewMacro(vtkPrismCubeAxesRepresentation);
//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::vtkPrismCubeAxesRepresentation()
{
  this->PrismCubeAxesActor = vtkPrismCubeAxesActor::New();
  this->PrismCubeAxesActor->SetPickable(0);

}

//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::~vtkPrismCubeAxesRepresentation()
{
  this->PrismCubeAxesActor->Delete();
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetVisibility(bool val)
{
  this->vtkPVDataRepresentation::SetVisibility(val);
  this->CubeAxesActor->SetVisibility(0);
  this->PrismCubeAxesActor->SetVisibility(val? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetColor(double r, double g, double b)
{
  this->PrismCubeAxesActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
bool vtkPrismCubeAxesRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    pvview->GetRenderer()->AddActor(this->PrismCubeAxesActor);
    this->PrismCubeAxesActor->SetCamera(pvview->GetActiveCamera());
    this->View = pvview;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPrismCubeAxesRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    pvview->GetRenderer()->RemoveActor(this->PrismCubeAxesActor);
    this->PrismCubeAxesActor->SetCamera(NULL);
    this->View = NULL;
    return true;
    }
  this->View = NULL;
  return false;
}


//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::UpdateBounds()
{
  if (this->BoundsUpdateTime < this->GetMTime())
    {
    if (this->View)
      {
      // This is a complex code that ensures that all processes end up with the
      // max bounds. This includes the client, data-server, render-server nodes.
      this->View->SynchronizeBounds(this->DataBounds);
      }
    this->BoundsUpdateTime.Modified();
    }
  double *scale = this->Scale;
  double *position = this->Position;
  double *rotation = this->Orientation;
  double bds[6];
  if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 ||
    position[0] != 0.0 || position[1] != 0.0 || position[2] != 0.0 ||
    rotation[0] != 0.0 || rotation[1] != 0.0 || rotation[2] != 0.0)
    {
    const double *bounds = this->DataBounds;
    vtkSmartPointer<vtkTransform> transform =
      vtkSmartPointer<vtkTransform>::New();
    transform->Translate(position);
    transform->RotateZ(rotation[2]);
    transform->RotateX(rotation[0]);
    transform->RotateY(rotation[1]);
    transform->Scale(scale);
    vtkBoundingBox bbox;
    int i, j, k;
    double origX[3], x[3];

    for (i = 0; i < 2; i++)
      {
      origX[0] = bounds[i];
      for (j = 0; j < 2; j++)
        {
        origX[1] = bounds[2 + j];
        for (k = 0; k < 2; k++)
          {
          origX[2] = bounds[4 + k];
          transform->TransformPoint(origX, x);
          bbox.AddPoint(x);
          }
        }
      }
    bbox.GetBounds(bds);
    }
  else
    {
    memcpy(bds, this->DataBounds, sizeof(double)*6);
    }

  //overload bounds with the active custom bounds
  for ( int i=0; i < 3; ++i)
    {
    int pos = i * 2;
    if ( this->CustomBoundsActive[i] )
      {
      bds[pos]=this->CustomBounds[pos];
      bds[pos+1]=this->CustomBounds[pos+1];
      }
    }
  this->PrismCubeAxesActor->SetBounds(bds);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//***************************************************************************
// Forwarded to internal vtkPrismCubeAxesActor
//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetLabelRanges(
  double a, double b, double c, double d, double e, double f)
{
  this->PrismCubeAxesActor->SetLabelRanges(a, b, c, d, e, f);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetFlyMode(int val)
{
  this->PrismCubeAxesActor->SetFlyMode(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetInertia(int val)
{
  this->PrismCubeAxesActor->SetInertia(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetCornerOffset(double val)
{
  this->PrismCubeAxesActor->SetCornerOffset(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetTickLocation(int val)
{
  this->PrismCubeAxesActor->SetTickLocation(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetXTitle(const char* val)
{
  this->PrismCubeAxesActor->SetXTitle(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetXAxisVisibility(int val)
{
  this->PrismCubeAxesActor->SetXAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetXAxisTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetXAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetXAxisMinorTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetXAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetDrawXGridlines(int val)
{
  this->PrismCubeAxesActor->SetDrawXGridlines(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetYTitle(const char* val)
{
  this->PrismCubeAxesActor->SetYTitle(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetYAxisVisibility(int val)
{
  this->PrismCubeAxesActor->SetYAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetYAxisTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetYAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetYAxisMinorTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetYAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetDrawYGridlines(int val)
{
  this->PrismCubeAxesActor->SetDrawYGridlines(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetZTitle(const char* val)
{
  this->PrismCubeAxesActor->SetZTitle(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetZAxisVisibility(int val)
{
  this->PrismCubeAxesActor->SetZAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetZAxisTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetZAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetZAxisMinorTickVisibility(int val)
{
  this->PrismCubeAxesActor->SetZAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetDrawZGridlines(int val)
{
  this->PrismCubeAxesActor->SetDrawZGridlines(val);
}
