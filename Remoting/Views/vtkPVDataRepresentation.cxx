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
#include "vtkLogger.h"
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
  this->CacheKey = 0.0;

  this->UniqueIdentifier = 0;

  this->HasTemporalPipeline = false;
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation::~vtkPVDataRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::SetInputConnection(port, input);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  this->Superclass::AddInputConnection(port, input);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetUpdateTime(double time)
{
  if (!this->UpdateTimeValid || (this->UpdateTimeValid && (this->UpdateTime != time)))
  {
    this->UpdateTime = time;
    this->UpdateTimeValid = true;

    // Call MarkModified() only when the timestep has indeed changed.
    if (this->HasTemporalPipeline && !this->GetNeedsUpdate())
    {
      // UpdateTimeChangedEvent is fired to let the proxy know that the pipeline
      // will be executed due to time change. This helps the proxy layer updated
      // state up the pipeline to note the potential re-execution of the
      // upstream pipeline, which otherwise it has no clue since the pipeline
      // isn't being executed due to explicit property modification.
      this->InvokeEvent(vtkPVDataRepresentation::UpdateTimeChangedEvent);
      this->MarkModified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::ResetUpdateTime()
{
  if (this->UpdateTimeValid)
  {
    this->UpdateTimeValid = false;
  }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVDataRepresentation::CreateDefaultExecutive()
{
  return vtkPVDataRepresentationPipeline::New();
}

//----------------------------------------------------------------------------
double vtkPVDataRepresentation::GetCacheKey() const
{
  return this->ForceUseCache ? this->ForcedCacheKey : this->CacheKey;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation*, vtkInformation*)
{
  assert("We must have an ID at that time" && this->UniqueIdentifier);
  assert(this->GetExecutive()->IsA("vtkPVDataRepresentationPipeline"));
  assert(this->GetVisibility());

  if (request == vtkPVView::REQUEST_UPDATE())
  {
    auto executive = vtkPVDataRepresentationPipeline::SafeDownCast(this->GetExecutive());
    assert(executive);

    // The representation hasn't been marked modified since last update. In that
    // case, we skip the update entirely since there's nothing to update.
    if (!executive->GetNeedsUpdate())
    {
      return 1;
    }

    auto pvview = vtkPVView::SafeDownCast(this->View);
    assert(pvview);

    // The representation needs to update. To support caching for playing
    // animation, we need to save the data we'll prepare in this update request
    // using a cache-key. That key is simply the view's current key. If nothing
    // was changed in the representation's input since last update, then we
    // won't get here and in that case we will continue to use the old
    // cache-key. This neat little trick is crucial to ensure that we don't
    // update representations that are not affected by the animation.
    this->CacheKey = pvview->GetCacheKey();

    // Now, check with the view, if the data was already cached. If so, we don't
    // need to update and can skip it.
    if (pvview->IsCached(this))
    {
      // update is needed, but we're skipping it since we have already cached
      // the update result.
      this->InvokeEvent(vtkPVDataRepresentation::SkippedUpdateDataEvent);
      executive->SetNeedsUpdate(false);
      return 1;
    }

    this->Update();
    executive->SetNeedsUpdate(false);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::MarkModified()
{
  this->Modified();

  if (this->HasExecutive())
  {
    auto executive = vtkPVDataRepresentationPipeline::SafeDownCast(this->GetExecutive());
    if (!executive->GetNeedsUpdate())
    {
      executive->SetNeedsUpdate(true);
      vtkLogF(TRACE, "MarkModified %s", this->LogName.c_str());
    }
  }

  // let the view know that representation has been modified;
  // the view may use this information to determine if cache keeps to be cleared
  // for the representation.
  if (auto pvview = vtkPVView::SafeDownCast(this->View))
  {
    pvview->ClearCache(this);
  }
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
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // cout << "Updated: " << this->LogName << endl;
  bool is_temporal = false;
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      auto inInfo = inputVector[cc]->GetInformationObject(kk);
      using SDDP = vtkStreamingDemandDrivenPipeline;
      is_temporal =
        is_temporal || inInfo->Has(SDDP::TIME_STEPS()) || inInfo->Has(SDDP::TIME_RANGE());
    }
  }
  this->HasTemporalPipeline = is_temporal;

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
bool vtkPVDataRepresentation::GetNeedsUpdate()
{
  auto executive = this->HasExecutive()
    ? vtkPVDataRepresentationPipeline::SafeDownCast(this->GetExecutive())
    : nullptr;
  return executive ? executive->GetNeedsUpdate() : false;
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
  os << indent << "CacheKey: " << this->CacheKey << endl;
}
