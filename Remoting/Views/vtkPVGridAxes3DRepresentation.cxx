/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGridAxes3DRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVGridAxes3DRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBoundingBox.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMolecule.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPVConfig.h"
#include "vtkPVGridAxes3DActor.h"
#include "vtkPVRenderView.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVGridAxes3DRepresentation);
//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetGridAxes(vtkPVGridAxes3DActor* gridAxes)
{
  if (gridAxes == this->GridAxes)
  {
    return;
  }

  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(this->GetView());
  if (rview && this->GridAxes)
  {
    rview->GetRenderer()->RemoveActor(this->GridAxes);
  }

  if (this->GridAxes)
  {
    this->GridAxes->UnRegister(this);
  }

  this->GridAxes = gridAxes;

  if (rview && this->GridAxes)
  {
    rview->GetRenderer()->AddActor(this->GridAxes);
  }

  if (this->GridAxes)
  {
    this->GridAxes->Register(this);
  }

  this->UpdateVisibility();

  this->MarkModified();
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetVisibility(bool vis)
{
  this->Superclass::SetVisibility(vis);
  this->UpdateVisibility();
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetGridAxesVisibility(bool vis)
{
  if (vis != this->GridAxesVisibility)
  {
    this->GridAxesVisibility = vis;
    this->UpdateVisibility();
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetPosition(double pos[3])
{
  if (this->Position[0] != pos[0] || this->Position[1] != pos[1] || this->Position[2] != pos[2])
  {
    std::copy(pos, pos + 3, this->Position);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetPosition(double x, double y, double z)
{
  double tmp[3] = { x, y, z };
  this->SetPosition(tmp);
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetScale(double scale[3])
{
  if (this->Scale[0] != scale[0] || this->Scale[1] != scale[1] || this->Scale[2] != scale[2])
  {
    std::copy(scale, scale + 3, this->Scale);
    this->MarkModified();
  }
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::SetScale(double x, double y, double z)
{
  double tmp[3] = { x, y, z };
  this->SetScale(tmp);
}

//------------------------------------------------------------------------------
int vtkPVGridAxes3DRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo) || !this->GridAxes)
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->DummyPolyData);
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
      /* deliver_to_client */ true, /* gather_before_delivery */ false);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

    vtkPolyData* dummyDataSet = nullptr;
    if (producerPort)
    {
      dummyDataSet = vtkPolyData::SafeDownCast(
        producerPort->GetProducer()->GetOutputDataObject(producerPort->GetIndex()));
    }
    double bounds[6];
    if (dummyDataSet)
    {
      dummyDataSet->GetBounds(bounds);
    }
    else
    {
      vtkMath::UninitializeBounds(bounds);
    }
    this->GridAxes->SetTransformedBounds(bounds);

    // Render during the opaque pass if rendering is distributed and we're not
    // using ordered compositing. The default depth compositing by IceT will
    // not detect the default translucent text, since the translucent pass
    // doesn't write to the depth buffer.
    bool forceOpaque = false;
    vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(this->GetView());
    if (rview)
    {
      forceOpaque = (rview->GetUseDistributedRenderingForLODRender() ||
                      rview->GetUseDistributedRenderingForRender()) &&
        !rview->GetUseOrderedCompositing();
    }
    this->GridAxes->SetForceOpaque(forceOpaque);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkPVGridAxes3DRepresentation::vtkPVGridAxes3DRepresentation()
  : GridAxesVisibility(false)
  , GridAxes(nullptr)
{
  std::fill(this->Position, this->Position + 3, 0.);
  std::fill(this->Scale, this->Scale + 3, 1.);
}

//------------------------------------------------------------------------------
vtkPVGridAxes3DRepresentation::~vtkPVGridAxes3DRepresentation()
{
  this->SetGridAxes(nullptr);
}

//------------------------------------------------------------------------------
int vtkPVGridAxes3DRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPVGridAxes3DRepresentation::RequestData(
  vtkInformation* req, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  this->DummyPolyData->Initialize();

  if (inInfoVec[0]->GetNumberOfInformationObjects() == 1)
  {
    double bounds[6];
    vtkDataSet* ds = vtkDataSet::GetData(inInfoVec[0], 0);
    vtkCompositeDataSet* cds = vtkCompositeDataSet::GetData(inInfoVec[0], 0);
    vtkMolecule* mol = vtkMolecule::SafeDownCast(
      inInfoVec[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(
      inInfoVec[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

    if (ds)
    {
      ds->GetBounds(bounds);
    }
    else if (cds)
    {
      vtkCompositeDataIterator* iter = cds->NewIterator();
      vtkBoundingBox bbox;
      double dsBounds[6];
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
        {
          ds->GetBounds(dsBounds);
          bbox.AddBounds(dsBounds);
        }
      }
      iter->Delete();
      bbox.GetBounds(bounds);
    }
    else if (mol)
    {
      mol->GetBounds(bounds);
    }
    else if (htg)
    {
      htg->GetBounds(bounds);
    }
    else
    {
      vtkMath::UninitializeBounds(bounds);
    }

    vtkMultiProcessController* mpc = vtkMultiProcessController::GetGlobalController();
    if (mpc && mpc->GetNumberOfProcesses() > 1)
    {
      if (!vtkMath::AreBoundsInitialized(bounds))
      {
        // If the local process's bounds are invalid, ensure that they'll be
        // ignored by the MAX reduction.
        std::fill(bounds, bounds + 6, VTK_DOUBLE_MIN);
      }
      else
      {
        // Otherwise, negate the minima so we can do MAX reduction:
        bounds[0] = -bounds[0];
        bounds[2] = -bounds[2];
        bounds[4] = -bounds[4];
      }

      // Reduce the bounds across all processes
      double gBounds[6];
      if (!mpc->AllReduce(bounds, gBounds, 6, vtkCommunicator::MAX_OP))
      {
        vtkErrorMacro("Bounds reduction failed.");
        return 0;
      }

      // Copy to bounds array, correcting the negated minima:
      bounds[0] = -gBounds[0];
      bounds[1] = gBounds[1];
      bounds[2] = -gBounds[2];
      bounds[3] = gBounds[3];
      bounds[4] = -gBounds[4];
      bounds[5] = gBounds[5];
    }

    if (vtkMath::AreBoundsInitialized(bounds))
    {
      // Account for transform info:
      for (int i = 0; i < 3; ++i)
      {
        bounds[2 * i] = bounds[2 * i] * this->Scale[i] + this->Position[i];
        bounds[2 * i + 1] = bounds[2 * i + 1] * this->Scale[i] + this->Position[i];
      }

      // Create the dataset:
      vtkPoints* points = vtkPoints::New();
      this->DummyPolyData->SetPoints(points);
      points->Delete();

      points->InsertNextPoint(bounds[0], bounds[2], bounds[4]);
      points->InsertNextPoint(bounds[1], bounds[3], bounds[5]);
    }
  }

  this->DummyPolyData->Modified();

  return this->Superclass::RequestData(req, inInfoVec, outInfoVec);
}

//------------------------------------------------------------------------------
bool vtkPVGridAxes3DRepresentation::AddToView(vtkView* view)
{
  if (this->GridAxes)
  {
    vtkPVRenderView* rView = vtkPVRenderView::SafeDownCast(view);
    if (rView)
    {
      rView->GetRenderer()->AddActor(this->GridAxes);
    }
  }
  return this->Superclass::AddToView(view);
}

//------------------------------------------------------------------------------
bool vtkPVGridAxes3DRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GridAxes)
  {
    vtkPVRenderView* rView = vtkPVRenderView::SafeDownCast(view);
    if (rView)
    {
      rView->GetRenderer()->RemoveActor(this->GridAxes);
    }
  }
  return this->Superclass::RemoveFromView(view);
}

//------------------------------------------------------------------------------
void vtkPVGridAxes3DRepresentation::UpdateVisibility()
{
  if (this->GridAxes)
  {
    this->GridAxes->SetVisibility((this->GetVisibility() && this->GridAxesVisibility) ? 1 : 0);
  }
}
