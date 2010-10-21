/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleParallelStrategy.h"

#include "vtkClientServerStream.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleParallelStrategy);
//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::vtkSMSimpleParallelStrategy()
{
  this->PostCollectUpdateSuppressor = 0;
  this->Collect = 0;

  this->PostCollectUpdateSuppressorLOD = 0;
  this->CollectLOD = 0;

  this->UseCompositing = false;

  this->LODClientRender = false;
  this->LODClientCollect = true;

  this->CollectedDataValid = false;
  this->CollectedLODDataValid = false;
}

//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::~vtkSMSimpleParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();
  this->UpdateSuppressor->SetServers(vtkProcessModule::DATA_SERVER);
  if (this->UpdateSuppressorLOD)
    {
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::DATA_SERVER);
    }

  this->Collect = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Collect"));
  this->PostCollectUpdateSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PostCollectUpdateSuppressor")); 

  this->CollectLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));
  this->PostCollectUpdateSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PostCollectUpdateSuppressorLOD"));

  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->PostCollectUpdateSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  if (this->CollectLOD)
    {
    this->CollectLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    this->PostCollectUpdateSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else
    {
    this->SetEnableLOD(false);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  // Extend the superclass's pipeline as follows
  //    SUPERCLASS --> Collect --> PostCollectUpdateSuppressor 
  this->CreatePipelineInternal(this->Superclass::GetOutput(), 0,
                               this->Collect, 
                               this->PostCollectUpdateSuppressor);

}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->Superclass::CreateLODPipeline(input, outputport);

  // Extend the superclass's pipeline as follows
  //    SUPERCLASS --> CollectLOD --> PostCollectUpdateSuppressorLOD 
  this->CreatePipelineInternal(this->Superclass::GetLODOutput(), 0,
                               this->CollectLOD, 
                               this->PostCollectUpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* input, int outputport,
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  input->CreateOutputPorts();
  const char* data_class_name =
    input->GetOutputPort(outputport)->GetDataClassName();
  int data_type_id =
    vtkDataObjectTypes::GetTypeIdFromClassName(data_class_name);

  this->Connect(input, collect, "Input", outputport);
  this->Connect(collect, updatesuppressor);

  // Now we need to set up some default parameters on these filters.
  
  // Collect filter needs the MPIMToNSocketConnection to communicate between
  // render server and data server nodes.
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetMPIMToNSocketConnection"
          << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
          << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << collect->GetID()
         << "SetOutputDataType"
         << data_type_id
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  // Set the server flag on the collect filter to correctly identify each
  // processes.
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetServerToRenderServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetServerToDataServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::DATA_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetServerToClient"
          << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << collect->GetID()
         << "SetOutputDataType"
         << data_type_id
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}


//----------------------------------------------------------------------------
int vtkSMSimpleParallelStrategy::GetMoveMode()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.
  bool usecompositing = this->GetUseCompositing();
  // cout << "usecompositing: " << usecompositing << endl;

  int move_mode = 0;
  if (usecompositing)
    {
    move_mode = vtkMPIMoveData::PASS_THROUGH;
    // cout << "PASS_THROUGH" << endl;
    }
  else
    {
    if (this->LODClientRender) 
      {
      // in tile-display mode.
      move_mode = vtkMPIMoveData::CLONE;
      // cout << "CLONE" << endl;
      }
    else
      {
      move_mode = vtkMPIMoveData::COLLECT;
      // cout << "COLLECT" << endl;
      }
    }
  return move_mode;
}

//----------------------------------------------------------------------------
int vtkSMSimpleParallelStrategy::GetLODMoveMode()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->GetUseCompositing();

  int move_mode = 0;
  if (usecompositing)
    {
    if (this->LODClientRender)
      {
      // In tile-display mode, we want the collected data on the client side 
      // for rendering as well.
      move_mode = vtkMPIMoveData::COLLECT_AND_PASS_THROUGH;
      }
    else
      {
      move_mode = vtkMPIMoveData::PASS_THROUGH;
      }
    // cout << "LOD PASS_THROUGH" << endl;
    }
  else
    {
    if (this->LODClientRender) 
      {
      // in tile-display mode.
      move_mode = vtkMPIMoveData::CLONE;
      // cout << "LOD CLONE" << endl;
      }
    else
      {
      move_mode = vtkMPIMoveData::COLLECT;
      // cout << "LOD COLLECT" << endl;
      }
    }
  return move_mode;
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdatePipeline()
{
  if (this->vtkSMSimpleParallelStrategy::GetDataValid())
    {
    return;
    }

  this->Superclass::UpdatePipeline();

  vtkSMPropertyHelper(this->Collect, "MoveMode").Set(this->GetMoveMode()); 
  this->Collect->UpdateProperty("MoveMode");

  // It is essential to mark the Collect filter explicitly modified.
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->Collect->GetID()
          << "Modified"
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Collect->GetServers(), stream);

  this->PostCollectUpdateSuppressor->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->PostCollectUpdateSuppressor->UpdatePipeline();
  this->CollectedDataValid = true;
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdateLODPipeline()
{
  if (this->vtkSMSimpleParallelStrategy::GetLODDataValid())
    {
    return;
    }

  this->Superclass::UpdateLODPipeline();

  vtkSMPropertyHelper(this->CollectLOD, "MoveMode").Set(this->GetLODMoveMode());
  this->CollectLOD->UpdateProperty("MoveMode");

  // It is essential to mark the Collect filter explicitly modified.
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CollectLOD->GetID()
          << "Modified"
          << vtkClientServerStream::End;

  // If LODClientCollect is false, then we need to ensure that the collect
  // filter does not deliver full data to the client. 
  stream  << vtkClientServerStream::Invoke
          << this->CollectLOD->GetID()
          << "SetDeliverOutlineToClient"
          << (this->LODClientCollect? 0 : 1)
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->CollectLOD->GetServers(), stream);

  this->PostCollectUpdateSuppressorLOD->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->PostCollectUpdateSuppressorLOD->UpdatePipeline();
  this->CollectedLODDataValid = true;
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::SetUseCompositing(bool compositing)
{
  if (this->UseCompositing != compositing)
    {
    this->UseCompositing = compositing;
    
    this->InvalidatePipeline();
    this->InvalidateLODPipeline();
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::SetLODClientCollect(bool collect)
{
  if (this->LODClientCollect != collect)
    {
    this->LODClientCollect = collect;
    this->InvalidateLODPipeline();
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::SetLODClientRender(bool render)
{
  if (this->LODClientRender != render)
    {
    this->LODClientRender = render;
    this->InvalidateLODPipeline();
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::ProcessViewInformation()
{
  if (this->ViewInformation->Has(vtkSMRenderViewProxy::USE_COMPOSITING()))
    {
    this->SetUseCompositing(
      this->ViewInformation->Get(vtkSMRenderViewProxy::USE_COMPOSITING())>0);
    }
  else
    {
    vtkErrorMacro("Missing Key: USE_COMPOSITING()");
    }

  if (this->ViewInformation->Has(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER()))
    {
    this->SetLODClientRender(this->ViewInformation->Get(
        vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())>0);
    }
  else
    {
    this->SetLODClientRender(false);
    }

  if (this->ViewInformation->Has(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_COLLECT()))
    {
    this->SetLODClientCollect(this->ViewInformation->Get(
        vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_COLLECT())>0);
    }
  else
    {
    this->SetLODClientCollect(true);
    }

  this->Superclass::ProcessViewInformation();
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
}


