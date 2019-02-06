/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataPipeline.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPVView.h"
#include "vtkSmartPointer.h"

#include <assert.h>
#include <map>

//----------------------------------------------------------------------------
vtkPVDataRepresentation::vtkPVDataRepresentation()
{
  this->Visibility = true;
  vtkExecutive* exec = this->CreateDefaultExecutive();
  this->SetExecutive(exec);
  exec->Delete();

  this->UpdateTimeValid = false;
  this->UpdateTime = 0.0;

  this->ForceUseCache = false;
  this->ForcedCacheKey = 0.0;

  this->NeedUpdate = true;

  this->UniqueIdentifier = 0;
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation::~vtkPVDataRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetUpdateTime(double time)
{
  if (!this->UpdateTimeValid || (this->UpdateTimeValid && (this->UpdateTime != time)))
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
  assert("We must have an ID at that time" && this->UniqueIdentifier);
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
unsigned int vtkPVDataRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  // Already initialized ?
  if (this->UniqueIdentifier)
  {
    return minIdAvailable;
  }

  assert(
    "Invalid Representation Id. Not enough reserved ids." && (maxIdAvailable >= minIdAvailable));
  (void)maxIdAvailable; // Prevent warning in Release build

  this->UniqueIdentifier = minIdAvailable;
  return (1 + this->UniqueIdentifier);
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
  this->NeedUpdate = false;

  // cout << "Updated: " << this->LogName << endl;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestUpdateTime(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      if (this->UpdateTimeValid)
      {
        inputVector[cc]->GetInformationObject(kk)->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->UpdateTime);
      }
    }
  }

  return 1;
}
//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ideally, extent and time information will come from the view in
  // REQUEST_UPDATE(), include view-time.
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  for (int cc = 0; cc < this->GetNumberOfInputPorts() && controller != NULL; cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      vtkInformation* info = inputVector[cc]->GetInformationObject(kk);
      info->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), controller->GetLocalProcessId());
      info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        controller->GetNumberOfProcesses());
      info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
      info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
      if (this->UpdateTimeValid)
      {
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->UpdateTime);
      }
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
vtkAlgorithmOutput* vtkPVDataRepresentation::GetInternalOutputPort(int port, int conn)
{
  vtkAlgorithmOutput* prevOutput = this->Superclass::GetInternalOutputPort(port, conn);
  if (!prevOutput)
  {
    return 0;
  }

  vtkTrivialProducer* prevProducer = static_cast<vtkTrivialProducer*>(prevOutput->GetProducer());
  if (prevProducer->IsA("vtkPVTrivialProducer"))
  {
    return prevOutput;
  }

  vtkDataObject* dobj = prevProducer->GetOutputDataObject(0);

  std::pair<int, int> p(port, conn);
  vtkPVTrivialProducer* tprod = vtkPVTrivialProducer::New();
  vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
  tprod->SetExecutive(exec);
  tprod->SetOutput(dobj);
  vtkInformation* portInfo = tprod->GetOutputPortInformation(0);
  portInfo->Set(vtkDataObject::DATA_TYPE_NAME(), dobj->GetClassName());
  this->SetInternalInput(port, conn, tprod);
  tprod->Delete();
  exec->Delete();

  return tprod->GetOutputPort();
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::AddToView(vtkView* view)
{
  if (this->View != NULL)
  {
    vtkWarningMacro("Added representation has a non-null 'View'. "
                    "A representation cannot be added to two views at the same time!");
  }
  this->View = view;
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::RemoveFromView(vtkView* view)
{
  if (this->View == view)
  {
    this->View = NULL;
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
vtkView* vtkPVDataRepresentation::GetView() const
{
  return this->View;
}

//----------------------------------------------------------------------------
double vtkPVDataRepresentation::GetCacheKey()
{
  if (this->ForceUseCache)
  {
    return this->ForcedCacheKey;
  }
  if (vtkPVView* pvview = vtkPVView::SafeDownCast(this->View))
  {
    return pvview->GetCacheKey();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::GetUseCache()
{
  if (this->ForceUseCache)
  {
    return true;
  }

  if (vtkPVView* pvview = vtkPVView::SafeDownCast(this->View))
  {
    return pvview->GetUseCache();
  }
  return false;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPVDataRepresentation::GetPipelineDataTime()
{
  if (auto executive = vtkPVDataRepresentationPipeline::SafeDownCast(this->GetExecutive()))
  {
    return executive->GetDataTime();
  }

  vtkErrorMacro("vtkPVDataRepresentationPipeline is expected!!!");
  return vtkMTimeType();
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "UpdateTimeValid: " << this->UpdateTimeValid << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;
  os << indent << "ForceUseCache: " << this->ForceUseCache << endl;
  os << indent << "ForcedCacheKey: " << this->ForcedCacheKey << endl;
}
