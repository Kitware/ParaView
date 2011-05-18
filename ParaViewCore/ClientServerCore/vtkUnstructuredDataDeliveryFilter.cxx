/*=========================================================================

  Program:   ParaView
  Module:    vtkUnstructuredDataDeliveryFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredDataDeliveryFilter.h"

#include "vtkCompositeMultiProcessController.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDummyController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>

class vtkUnstructuredDataDeliveryFilter::VoidPtrSet : public vtkstd::set<void*>
{
};

vtkStandardNewMacro(vtkUnstructuredDataDeliveryFilter);
//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::vtkUnstructuredDataDeliveryFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->PassThroughMoveData = vtkMPIMoveData::New();

  this->ServerMoveData = vtkMPIMoveData::New();
  this->ClientMoveData = vtkMPIMoveData::New();
  this->ClientMoveData->SetInputConnection(
    this->ServerMoveData->GetOutputPort());

  this->OutputDataType = VTK_VOID;
  this->SetOutputDataType(VTK_POLY_DATA);

  this->LODMode = false;
  this->DataMoveState = NO_WHERE;

  this->HandledClients = new VoidPtrSet();

  // Discover process type and setup communication ivars.
  this->InitializeForCommunication();
}

//----------------------------------------------------------------------------
vtkUnstructuredDataDeliveryFilter::~vtkUnstructuredDataDeliveryFilter()
{
  this->PassThroughMoveData->Delete();
  this->ServerMoveData->Delete();
  this->ClientMoveData->Delete();

  delete this->HandledClients;
  this->HandledClients = NULL;
}

//----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::SetOutputDataType(int type)
{
  if (this->OutputDataType != type)
    {
    this->OutputDataType = type;
    this->PassThroughMoveData->SetOutputDataType(type);
    this->ServerMoveData->SetOutputDataType(type);
    this->ClientMoveData->SetOutputDataType(type);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkDataObject *output = vtkDataObject::GetData(outputVector, 0);
  if (!output || !output->IsA(
      vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataType)))
    {
    output = vtkDataObjectTypes::NewDataObject(this->OutputDataType);
    if (!output)
      {
      return 0;
      }
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
      output->GetExtentType());
    output->FastDelete();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::InitializeForCommunication()
{
  this->PassThroughMoveData->InitializeForCommunicationForParaView();
  this->PassThroughMoveData->SetMoveModeToPassThrough();

  this->ServerMoveData->InitializeForCommunicationForParaView();
  this->ServerMoveData->SetClientDataServerSocketController(NULL);

  this->ClientMoveData->InitializeForCommunicationForParaView();
  vtkDummyController* cntr = vtkDummyController::New();
  this->ClientMoveData->SetController(cntr);
  cntr->FastDelete();

  this->ClientMoveData->SetMPIMToNSocketConnection(NULL);
}

//----------------------------------------------------------------------------
int vtkUnstructuredDataDeliveryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDataObject* input = (inputVector[0]->GetNumberOfInformationObjects() == 1)?
    vtkDataObject::GetData(inputVector[0], 0) : NULL;
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  if (this->UsePassThrough)
    {
    if  ((this->DataMoveState & PASS_THROUGH) == 0)
      {
      this->DataMoveState |= PASS_THROUGH;
      vtkSmartPointer<vtkDataObject> inputClone;
      if (input)
        {
        inputClone.TakeReference(input->NewInstance());
        inputClone->ShallowCopy(input);
        }
      this->PassThroughMoveData->SetInput(inputClone);
      }
    this->PassThroughMoveData->Update();
    output->ShallowCopy(
      this->PassThroughMoveData->GetOutputDataObject(0));
    }
  else
    {
    if ((this->DataMoveState & COLLECT_TO_ROOT) == 0)
      {
      this->DataMoveState |= COLLECT_TO_ROOT;
      if (input)
        {
        vtkSmartPointer<vtkDataObject> inputClone;
        inputClone.TakeReference(input->NewInstance());
        inputClone->ShallowCopy(input);
        this->ServerMoveData->SetInput(inputClone);
        this->ServerMoveData->Update();
        }
      }
    this->ClientMoveData->Update();
    output->ShallowCopy(
      this->ClientMoveData->GetOutputDataObject(0));
    vtkMultiProcessController* client_controller =
      this->ClientMoveData->GetClientDataServerSocketController();
    vtkCompositeMultiProcessController* ccontroller =
      vtkCompositeMultiProcessController::SafeDownCast(client_controller);
    if (ccontroller)
      {
      client_controller= ccontroller->GetActiveController();
      }
    this->HandledClients->insert(client_controller);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::Modified()
{
  this->ServerMoveData->Modified();
  this->ClientMoveData->Modified();
  this->PassThroughMoveData->Modified();
  this->DataMoveState = NO_WHERE;
  this->HandledClients->clear();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::ProcessViewRequest(vtkInformation* info)
{
  // This magical piece of code may need some explanation. The complication here
  // is to support multi-client mode with minimal data movement. To do that we
  // use 3 vtkMPIMoveData instances.
  // * this->PassThroughMoveData is used in PASS_THROUGH mode.
  // * this->ServerMoveData and this->ClientMoveData are used in any move mode
  //   that involves the client e.g. CLONE or COLLECT. Both these modes involve
  //   a purely server data-movement followed by a root-to-client movement and
  //   avoid doing the former repeatedly for more than 1 client, we split the
  //   pipeline to use 2 filters. We use an internal cache to determine if the
  //   geometry has already been delivered to a client.

  // When input data is modified, vtkUnstructuredDataDeliveryFilter::Modified()
  // is called which pretty much clears all the cache and marks all move-data
  // filters dirty.

  // This->DataMoveState is flag that helps us determine that locations to which
  // the data has been moved since the most recent
  // vtkUnstructuredDataDeliveryFilter::Modified() call.

  // In this function, based on the move-mode we first determine what
  // vtkMPIMoveData filter to use. Second, we mark the filter modified is it
  // needs to execute based on this->DataMoveState flag and the cache (in case
  // data needs to be delivered to client).

  int move_mode = -1;
  if (info->Has(vtkPVRenderView::DATA_DISTRIBUTION_MODE()))
    {
    move_mode = info->Get(vtkPVRenderView::DATA_DISTRIBUTION_MODE());
    }

  bool old_use_pass_through = this->UsePassThrough;
  if (move_mode == vtkMPIMoveData::PASS_THROUGH)
    {
    // RequestData must use this->PassThroughMoveData filter.
    this->UsePassThrough = true;
    if  ((this->DataMoveState & PASS_THROUGH) == 0)
      {
      // since this->DataMoveState doesn't say data has been "passed through"
      // even once since the most recent Modified() call, we force the
      // PassThroughMoveData (as well as this class' RequestData()) to execute
      // by marking this filter modified.
      this->PassThroughMoveData->Modified();
      }
    }
  else if (move_mode == vtkMPIMoveData::COLLECT || move_mode ==
    vtkMPIMoveData::CLONE)
    {
    // We are not using this->PassThroughMoveData.
    this->UsePassThrough = false;
    if ((this->DataMoveState & COLLECT_TO_ROOT) == 0)
      {
      // since this filter never operated in clone or collect mode, we don't
      // have the data available yet at the root node, so this->ServerMoveData
      // must execute. This just ensures that.
      this->ServerMoveData->Modified();
      this->ClientMoveData->Modified();
      }
    this->ServerMoveData->SetMoveMode(move_mode);
    this->ClientMoveData->SetMoveModeToCollect();
    vtkMultiProcessController* client_controller =
      this->ClientMoveData->GetClientDataServerSocketController();
    vtkCompositeMultiProcessController* ccontroller =
      vtkCompositeMultiProcessController::SafeDownCast(client_controller);
    if (ccontroller)
      {
      client_controller= ccontroller->GetActiveController();
      }

    // check if the cache tells us that the data has been delivered to the
    // current client, if not, we force the ClientMoveData to execute and
    // deliver the data.
    if (this->HandledClients->find(client_controller) ==
      this->HandledClients->end())
      {
      this->ClientMoveData->Modified();
      // don't add the client_controller to HandledClients list immediately, it
      // will be added when the data is actually delivered to the client
      // (in RequestData()).
      // this->HandledClients->insert(client_controller);
      }
    }
  else // if (move_mode == vtkMPIMoveData::COLLECT_AND_PASS_THROUGH)
    {
    // FIXME_COLLABORATION
    vtkWarningMacro("Not supported in collaborative mode");
    this->UsePassThrough = false;
    this->PassThroughMoveData->SetMoveMode(move_mode);
    }

  // if the move_mode has changed for the same client, then too we should mark
  // ourselves modified. This still won't result in any actual data movement
  // (unless needed), but ensures that output from correct internal filter is
  // passed along.
  if (old_use_pass_through != this->UsePassThrough)
    {
    this->Superclass::Modified();
    }

  if (this->UsePassThrough == false)
    {
    bool deliver_outline =
      (info->Has(vtkPVRenderView::DELIVER_OUTLINE_TO_CLIENT()) != 0);
    if (this->LODMode)
      {
      deliver_outline |=
        (info->Has(vtkPVRenderView::DELIVER_OUTLINE_TO_CLIENT_FOR_LOD())!=0);
      }
    if (deliver_outline)
      {
      this->ClientMoveData->SetDeliverOutlineToClient(1);
      }
    else
      {
      this->ClientMoveData->SetDeliverOutlineToClient(0);
      }
    }
}

//----------------------------------------------------------------------------
unsigned long vtkUnstructuredDataDeliveryFilter::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();

  unsigned long md_mtime = this->ServerMoveData->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;

  md_mtime = this->ClientMoveData->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;

  md_mtime = this->PassThroughMoveData->GetMTime();
  mtime = mtime > md_mtime? mtime : md_mtime;

  return mtime;
}

//----------------------------------------------------------------------------
void vtkUnstructuredDataDeliveryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

