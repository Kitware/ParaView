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
#include "vtkPVDataRepresentation.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVView.h"

#include <assert.h>
//----------------------------------------------------------------------------
vtkPVDataRepresentation::vtkPVDataRepresentation()
{
  this->Visibility = true;
  vtkExecutive* exec = this->CreateDefaultExecutive();
  this->SetExecutive(exec);
  exec->Delete();

  this->UpdateTimeValid = false;
  this->UpdateTime = 0.0;

  this->UseCache = false;
  this->CacheKey = 0.0;

  this->ForceUseCache = false;
  this->ForcedCacheKey = 0.0;

  this->NeedUpdate = true;
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation::~vtkPVDataRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetUpdateTime(double time)
{
  if (!this->UpdateTimeValid ||
    (this->UpdateTimeValid && (this->UpdateTime != time)))
    {
    this->UpdateTime = time;
    this->UpdateTimeValid = true;

    // Call MarkModified() only when the timestep has indeed changed.
    this->MarkModified();
    }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVDataRepresentation::CreateDefaultExecutive()
{
  return vtkPVDataRepresentationPipeline::New();
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation*, vtkInformation*)
{
  assert(this->GetExecutive()->IsA("vtkPVDataRepresentationPipeline"));
  if (this->GetVisibility() == false)
    {
    return 0;
    }

  if (request == vtkPVView::REQUEST_UPDATE())
    {
    this->Update();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::MarkModified()
{
  this->Modified();
  this->NeedUpdate = true;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*)
{
  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
  this->NeedUpdate = false;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ideally, extent and time information will come from the view in
  // REQUEST_UPDATE(), include view-time.
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  if (controller && inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkStreamingDemandDrivenPipeline* sddp =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
    sddp->SetUpdateExtent(inputVector[0]->GetInformationObject(0),
      controller->GetLocalProcessId(),
      controller->GetNumberOfProcesses(), 0);
    if (this->UpdateTimeValid)
      {
      sddp->SetUpdateTimeSteps(
        inputVector[0]->GetInformationObject(0),
        &this->UpdateTime, 1);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::GetUsingCacheForUpdate()
{
  if (this->GetUseCache())
    {
    return this->IsCached(this->GetCacheKey());
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "UpdateTimeValid: " << this->UpdateTimeValid << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;
  os << indent << "UseCache: " << this->UseCache << endl;
  os << indent << "CacheKey: " << this->CacheKey << endl;
  os << indent << "ForceUseCache: " << this->ForceUseCache << endl;
  os << indent << "ForcedCacheKey: " << this->ForcedCacheKey << endl;
}
