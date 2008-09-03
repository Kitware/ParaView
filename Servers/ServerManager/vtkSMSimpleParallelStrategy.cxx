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
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleParallelStrategy);
vtkCxxRevisionMacro(vtkSMSimpleParallelStrategy, "1.23");
//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::vtkSMSimpleParallelStrategy()
{
  this->PreCollectUpdateSuppressor = 0;
  this->Collect = 0;

  this->PreCollectUpdateSuppressorLOD = 0;
  this->CollectLOD = 0;

  this->UseCompositing = false;

  this->LODClientRender = false;
  this->LODClientCollect = true;
}

//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::~vtkSMSimpleParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->PreCollectUpdateSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreCollectUpdateSuppressor"));
  this->Collect = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Collect"));

  this->PreCollectUpdateSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreCollectUpdateSuppressorLOD"));
  this->CollectLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));

  this->PreCollectUpdateSuppressor->SetServers(vtkProcessModule::DATA_SERVER);
  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  if (this->CollectLOD)
    {
    this->PreCollectUpdateSuppressorLOD->SetServers(vtkProcessModule::DATA_SERVER);
    this->CollectLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
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
  // We set up the pipeline as such
  //    INPUT --> Collect --> UpdateSuppressor
  this->CreatePipelineInternal(input, outputport,
                               this->Collect, 
                               this->UpdateSuppressor);

  // Connect the PreCollectUpdateSuppressor to the input.
  this->Connect(input, this->PreCollectUpdateSuppressor, "Input", outputport);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  // We set up the pipeline as such
  //    INPUT --> LODDecimator --> Collect --> UpdateSuppressor
  this->Connect(input, this->LODDecimator, "Input", outputport);
  this->CreatePipelineInternal(this->LODDecimator, 0,
                               this->CollectLOD, 
                               this->UpdateSuppressorLOD);

  // Connect the PreCollectUpdateSuppressorLOD to the decimator.
  this->Connect(this->LODDecimator, this->PreCollectUpdateSuppressorLOD, "Input", 0);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* input, int outputport,
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(input, collect, "Input", outputport);
  this->Connect(collect, updatesuppressor);

  // Now we need to set up some default parameters on these filters.

  // Collect filter needs the socket controller use to communicate between
  // data-server root and the client.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetSocketController"
          << pm->GetConnectionClientServerID(this->ConnectionID)
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetSocketController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT_AND_SERVERS, stream);

  // Collect filter needs the MPIMToNSocketConnection to communicate between
  // render server and data server nodes.
  stream  << vtkClientServerStream::Invoke
          << collect->GetID()
          << "SetMPIMToNSocketConnection"
          << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
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
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}


//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdatePipeline()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->GetUseCompositing();
  // cout << "usecompositing: " << usecompositing << endl;

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("MoveMode"));

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
  
  ivp->SetElement(0, move_mode);
  this->Collect->UpdateProperty("MoveMode");

  // It is essential to mark the Collect filter explicitly modified.
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->Collect->GetID()
          << "Modified"
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Collect->GetServers(), stream);

  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdateLODPipeline()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->GetUseCompositing();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectLOD->GetProperty("MoveMode"));

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

  ivp->SetElement(0, move_mode);
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

  this->Superclass::UpdateLODPipeline();
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
void vtkSMSimpleParallelStrategy::GatherInformation(vtkPVInformation* info)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->GetUseCache())
    {
    // when using cache, if the cached time is available in the update
    // suppressor that caches, then we use the information from that update
    // suppressor, else we follow the usual procedure.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->UpdateSuppressor->GetID()
           << "IsCached"
           << this->CacheTime
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::DATA_SERVER_ROOT, stream);

    vtkClientServerStream values;
    int is_cached=false;
    if (pm->GetLastResult(this->ConnectionID,
        vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &values) &&
      values.GetArgument(0, 1, &is_cached) && 
      is_cached)
      {
      this->SomethingCached = true;
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UpdateSuppressor->GetProperty("CacheUpdate"));
      dvp->SetElement(0, this->CacheTime);
      this->UpdateSuppressor->UpdateProperty("CacheUpdate", 1);
      pm->GatherInformation(this->ConnectionID,
        vtkProcessModule::DATA_SERVER_ROOT,
        info,
        this->UpdateSuppressor->GetID());
      return;
      }
    }

  // Update the pipeline partially until before the Collect proxy
  this->PreCollectUpdateSuppressor->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->PreCollectUpdateSuppressor->UpdatePipeline();
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::DATA_SERVER_ROOT,
    info,
    this->PreCollectUpdateSuppressor->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::GatherLODInformation(vtkPVInformation* info)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  // First update the pipeline partially until before the Collect proxy
  if (this->GetUseCache())
    {
    // when using cache, if the cached time is available in the update
    // suppressor that caches, then we use the information from that update
    // suppressor, else we follow the usual procedure.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->UpdateSuppressorLOD->GetID()
           << "IsCached"
           << this->CacheTime
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::DATA_SERVER_ROOT, stream);

    vtkClientServerStream values;
    int is_cached=false;
    if (pm->GetLastResult(this->ConnectionID,
        vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &values) &&
      values.GetArgument(0, 1, &is_cached) && 
      is_cached)
      {
      this->SomethingCached = true;
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->UpdateSuppressorLOD->GetProperty("CacheUpdate"));
      dvp->SetElement(0, this->CacheTime);
      this->UpdateSuppressorLOD->UpdateProperty("CacheUpdate", 1);
      pm->GatherInformation(this->ConnectionID,
        vtkProcessModule::DATA_SERVER_ROOT,
        info,
        this->UpdateSuppressorLOD->GetID());
      return;
      }
    }

  // Update the pipeline partially until before the Collect proxy
  this->PreCollectUpdateSuppressorLOD->InvokeCommand("ForceUpdate");
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::DATA_SERVER_ROOT,
    info,
    this->PreCollectUpdateSuppressorLOD->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
}


