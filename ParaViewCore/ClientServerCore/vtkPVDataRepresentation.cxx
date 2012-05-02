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
  for (int cc=0; cc < this->GetNumberOfInputPorts() && controller != NULL; cc++)
    {
    for (int kk=0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
      {
      vtkStreamingDemandDrivenPipeline* sddp =
        vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
      sddp->SetUpdateExtent(inputVector[cc]->GetInformationObject(kk),
        controller->GetLocalProcessId(),
        controller->GetNumberOfProcesses(), /*ghost-levels*/ 0);
      inputVector[cc]->GetInformationObject(kk)->Set(
        vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
      if (this->UpdateTimeValid)
        {
        sddp->SetUpdateTimeStep(
          inputVector[cc]->GetInformationObject(kk),
          this->UpdateTime);
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
vtkAlgorithmOutput* vtkPVDataRepresentation::GetInternalOutputPort(int port,
                                                                   int conn)
{
  vtkAlgorithmOutput* prevOutput =
    this->Superclass::GetInternalOutputPort(port, conn);
  if (!prevOutput)
    {
    return 0;
    }

  vtkTrivialProducer* prevProducer = static_cast<vtkTrivialProducer*>(
    prevOutput->GetProducer());
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
