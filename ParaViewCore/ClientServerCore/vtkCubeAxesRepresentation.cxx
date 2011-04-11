/*=========================================================================

  Program:   ParaView
  Module:    vtkCubeAxesRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCubeAxesRepresentation.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCubeAxesActor.h"
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

vtkStandardNewMacro(vtkCubeAxesRepresentation);
//----------------------------------------------------------------------------
vtkCubeAxesRepresentation::vtkCubeAxesRepresentation()
{
  this->CubeAxesActor = vtkCubeAxesActor::New();
  this->CubeAxesActor->SetPickable(0);

  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->CustomBounds[0] = this->CustomBounds[2] = this->CustomBounds[4] = 0.0;
  this->CustomBounds[1] = this->CustomBounds[3] = this->CustomBounds[5] = 1.0;
  this->CustomBoundsActive[0] = 0;
  this->CustomBoundsActive[1] = 0;
  this->CustomBoundsActive[2] = 0;
}

//----------------------------------------------------------------------------
vtkCubeAxesRepresentation::~vtkCubeAxesRepresentation()
{
  this->CubeAxesActor->Delete();
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->CubeAxesActor->SetVisibility(val? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetColor(double r, double g, double b)
{
  this->CubeAxesActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
bool vtkCubeAxesRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    pvview->GetRenderer()->AddActor(this->CubeAxesActor);
    this->CubeAxesActor->SetCamera(pvview->GetActiveCamera());
    this->View = pvview;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkCubeAxesRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    pvview->GetRenderer()->RemoveActor(this->CubeAxesActor);
    this->CubeAxesActor->SetCamera(NULL);
    this->View = NULL;
    return true;
    }
  this->View = NULL;
  return false;
}

//----------------------------------------------------------------------------
int vtkCubeAxesRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkCubeAxesRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo,
  vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_PREPARE_FOR_RENDER())
    {
    this->UpdateBounds();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCubeAxesRepresentation::RequestData(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
    if (ds)
      {
      ds->GetBounds(this->DataBounds);
      }
    else
      {
      vtkCompositeDataIterator* iter = cd->NewIterator();
      vtkBoundingBox bbox;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
        {
        ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
          {
          double bds[6];
          ds->GetBounds(bds);
          if (vtkMath::AreBoundsInitialized(bds))
            {
            bbox.AddBounds(bds);
            }
          }
        }
      iter->Delete();
      bbox.GetBounds(this->DataBounds);
      }
    }

  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::UpdateBounds()
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
  this->CubeAxesActor->SetBounds(bds);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//***************************************************************************
// Forwarded to internal vtkCubeAxesActor
void vtkCubeAxesRepresentation::SetFlyMode(int val)
{
  this->CubeAxesActor->SetFlyMode(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetInertia(int val)
{
  this->CubeAxesActor->SetInertia(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetCornerOffset(double val)
{
  this->CubeAxesActor->SetCornerOffset(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetTickLocation(int val)
{
  this->CubeAxesActor->SetTickLocation(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetXTitle(const char* val)
{
  this->CubeAxesActor->SetXTitle(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetXAxisVisibility(int val)
{
  this->CubeAxesActor->SetXAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetXAxisTickVisibility(int val)
{
  this->CubeAxesActor->SetXAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetXAxisMinorTickVisibility(int val)
{
  this->CubeAxesActor->SetXAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetDrawXGridlines(int val)
{
  this->CubeAxesActor->SetDrawXGridlines(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetYTitle(const char* val)
{
  this->CubeAxesActor->SetYTitle(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetYAxisVisibility(int val)
{
  this->CubeAxesActor->SetYAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetYAxisTickVisibility(int val)
{
  this->CubeAxesActor->SetYAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetYAxisMinorTickVisibility(int val)
{
  this->CubeAxesActor->SetYAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetDrawYGridlines(int val)
{
  this->CubeAxesActor->SetDrawYGridlines(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetZTitle(const char* val)
{
  this->CubeAxesActor->SetZTitle(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetZAxisVisibility(int val)
{
  this->CubeAxesActor->SetZAxisVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetZAxisTickVisibility(int val)
{
  this->CubeAxesActor->SetZAxisTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetZAxisMinorTickVisibility(int val)
{
  this->CubeAxesActor->SetZAxisMinorTickVisibility(val);
}

//----------------------------------------------------------------------------
void vtkCubeAxesRepresentation::SetDrawZGridlines(int val)
{
  this->CubeAxesActor->SetDrawZGridlines(val);
}
