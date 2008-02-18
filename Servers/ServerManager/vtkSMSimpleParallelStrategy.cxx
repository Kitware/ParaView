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

inline int vtkSMSimpleParallelStrategyGetInt(vtkSMProxy* proxy, 
  const char* pname, int default_value)
{
  if (proxy && pname)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty(pname));
    if (ivp)
      {
      return ivp->GetElement(0);
      }
    }
  return default_value;
}

vtkStandardNewMacro(vtkSMSimpleParallelStrategy);
vtkCxxRevisionMacro(vtkSMSimpleParallelStrategy, "1.18");
//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::vtkSMSimpleParallelStrategy()
{
  this->PreCollectUpdateSuppressor = 0;
  this->Collect = 0;
  this->PreDistributorSuppressor = 0;
  this->Distributor = 0;

  this->PreCollectUpdateSuppressorLOD = 0;
  this->CollectLOD = 0;
  this->PreDistributorSuppressorLOD = 0;
  this->DistributorLOD = 0;

  this->UseCompositing = false;
  this->UseOrderedCompositing = false;

  this->KdTree = 0;
  this->LODClientRender = false;
  this->LODClientCollect = true;
}

//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::~vtkSMSimpleParallelStrategy()
{
  this->SetKdTree(0);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->PreCollectUpdateSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreCollectUpdateSuppressor"));
  this->Collect = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Collect"));
  this->PreDistributorSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressor"));
  this->Distributor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Distributor"));

  this->PreCollectUpdateSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreCollectUpdateSuppressorLOD"));
  this->CollectLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));
  this->PreDistributorSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressorLOD"));
  this->DistributorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("DistributorLOD"));

  this->PreCollectUpdateSuppressor->SetServers(vtkProcessModule::DATA_SERVER);
  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->PreDistributorSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Distributor->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->CollectLOD && this->PreDistributorSuppressorLOD &&
    this->DistributorLOD)
    {
    this->PreCollectUpdateSuppressorLOD->SetServers(vtkProcessModule::DATA_SERVER);
    this->CollectLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    this->PreDistributorSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    this->DistributorLOD->SetServers(vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    this->SetEnableLOD(false);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->SetKdTree(this->KdTree);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdateDistributedData()
{
  if (!this->DataValid)
    {
    // TODO: How to handle this when using cache?
    // We need to call this only if cache won't be used.
    this->PreDistributorSuppressor->InvokeCommand("ForceUpdate");
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->CreatePipelineInternal(input, outputport,
                               this->Collect, 
                               this->PreDistributorSuppressor,
                               this->Distributor,
                               this->UpdateSuppressor);

  // Connect the PreCollectUpdateSuppressor to the input.
  this->Connect(input, this->PreCollectUpdateSuppressor, "Input", outputport);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->Connect(input, this->LODDecimator, "Input", outputport);
  this->CreatePipelineInternal(this->LODDecimator, 0,
                               this->CollectLOD, 
                               this->PreDistributorSuppressorLOD,
                               this->DistributorLOD,
                               this->UpdateSuppressorLOD);

  // Connect the PreCollectUpdateSuppressorLOD to the decimator.
  this->Connect(this->LODDecimator, this->PreCollectUpdateSuppressorLOD, "Input", 0);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* input, int outputport,
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* predistributorsuppressor,
  vtkSMSourceProxy* distributor,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(input, collect, "Input", outputport);
  this->Connect(collect, predistributorsuppressor);

  // This sets the connection on the render server (since Distributor::Servers =
  // vtkProcessModule::RENDER_SERVER).
  this->Connect(predistributorsuppressor, distributor);

  // On Render Server, the Distributor is connected to the input of the
  // UpdateSuppressor. Since there are no distributors on the client
  // (or data server), we directly connect the PreDistributorSuppressor to the
  // UpdateSuppressor on the client and data server.

  stream  << vtkClientServerStream::Invoke
          << predistributorsuppressor->GetID()
          << "GetOutputPort" << 0
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << updatesuppressor->GetID()
          << "SetInputConnection" << 0
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);

  // Now send to the render server.
  // This order of sending first to CLIENT|DATA_SERVER and then to render server
  // ensures that the connections are set up correctly even when data server and
  // render server are the same.
  stream  << vtkClientServerStream::Invoke
          << distributor->GetID() 
          << "GetOutputPort" << 0
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << updatesuppressor->GetID()
          << "SetInputConnection" << 0 
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

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
  

  // Set the MultiProcessController on the distributor.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << distributor->GetID()
          << "SetController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

  // TODO: Set the distributor data type -- I don't think that's even necessary,
  // since the data type must be set only for type conversion and don't really
  // need any type conversion.
  
  // The PreDistributorSuppressor should not supress any updates, 
  // it's purpose is to force updates, not suppress any.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    predistributorsuppressor->GetProperty("Enabled"));
  ivp->SetElement(0, 0);
  predistributorsuppressor->UpdateVTKObjects();

  // The distributor does not do any distribution by default.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    distributor->GetProperty("PassThrough"));
  ivp->SetElement(0, 1);
  distributor->UpdateVTKObjects();
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

  if (!this->GetEnableLOD())
    {
    // LOD pipeline not present, treat full res pipeline as LOD.
    ivp->SetElement(0,
      usecompositing? 
      (this->LODClientRender? vtkMPIMoveData::CLONE: vtkMPIMoveData::PASS_THROUGH) : 
      vtkMPIMoveData::COLLECT);
    }
  else
    {
    ivp->SetElement(0,
      usecompositing? vtkMPIMoveData::PASS_THROUGH : vtkMPIMoveData::COLLECT);
    }
  this->Collect->UpdateProperty("MoveMode");

  // cout << "use ordered compositing: " << (usecompositing && this->UseOrderedCompositing)
  //  << endl;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Distributor->GetProperty("PassThrough"));
  ivp->SetElement(0,
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->Distributor->UpdateProperty("PassThrough");

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
  ivp->SetElement(0,
    usecompositing? 
    (this->LODClientRender? vtkMPIMoveData::CLONE: vtkMPIMoveData::PASS_THROUGH) : 
    vtkMPIMoveData::COLLECT);
  this->CollectLOD->UpdateProperty("MoveMode");

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DistributorLOD->GetProperty("PassThrough"));
  ivp->SetElement(0,
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->DistributorLOD->UpdateProperty("PassThrough");

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
void vtkSMSimpleParallelStrategy::SetUseOrderedCompositing(bool use)
{
  if (this->UseOrderedCompositing != use)
    {
    this->UseOrderedCompositing = use;

    // invalidate data, since distribution changed.
    this->InvalidatePipeline();
    this->InvalidateLODPipeline();
    }
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
void vtkSMSimpleParallelStrategy::SetKdTree(vtkSMProxy* proxy)
{
  vtkSetObjectBodyMacro(KdTree, vtkSMProxy, proxy);

  if (this->Distributor)
    {
    this->Connect(proxy, this->Distributor, "PKdTree");
    }

  if (this->DistributorLOD)
    {
    this->Connect(proxy, this->DistributorLOD, "PKdTree");
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

 if (this->ViewInformation->Has(vtkSMRenderViewProxy::USE_ORDERED_COMPOSITING()))
    {
    this->SetUseOrderedCompositing(
      this->ViewInformation->Get(
        vtkSMRenderViewProxy::USE_ORDERED_COMPOSITING())>0);
    }
  else
    {
    vtkErrorMacro("Missing Key: USE_ORDERED_COMPOSITING()");
    }

  if (this->ViewInformation->Has(vtkSMIceTCompositeViewProxy::KD_TREE()))
    {
    this->SetKdTree(vtkSMProxy::SafeDownCast(
        this->ViewInformation->Get(vtkSMIceTCompositeViewProxy::KD_TREE())));
    }
  else
    {
    // Don't warn if missing KD_TREE, since it's defined only by IceT views.
    //vtkErrorMacro("Missing Key: KD_TREE()");
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
  os << indent << "UseOrderedCompositing: " << this->UseOrderedCompositing
    << endl;
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
}


