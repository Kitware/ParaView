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
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIceTCompositeViewProxy.h"
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
vtkCxxRevisionMacro(vtkSMSimpleParallelStrategy, "1.10");
//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::vtkSMSimpleParallelStrategy()
{
  this->Collect = 0;
  this->PreDistributorSuppressor = 0;
  this->Distributor = 0;

  this->CollectLOD = 0;
  this->PreDistributorSuppressorLOD = 0;
  this->DistributorLOD = 0;

  this->UseCompositing = false;
  this->UseOrderedCompositing = false;

  this->KdTree = 0;
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

  this->Collect = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Collect"));
  this->PreDistributorSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressor"));
  this->Distributor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Distributor"));

  this->CollectLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));
  this->PreDistributorSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressorLOD"));
  this->DistributorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("DistributorLOD"));

  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->PreDistributorSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Distributor->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->CollectLOD && this->PreDistributorSuppressorLOD &&
    this->DistributorLOD)
    {
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

  this->UpdatePieceInformation(this->PreDistributorSuppressor);
  if (this->GetEnableLOD())
    {
    this->UpdatePieceInformation(this->PreDistributorSuppressorLOD);
    }

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
  ivp->SetElement(0, 
    usecompositing? vtkMPIMoveData::PASS_THROUGH : vtkMPIMoveData::COLLECT);
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
    usecompositing? vtkMPIMoveData::PASS_THROUGH : vtkMPIMoveData::COLLECT);
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
    this->Connect(proxy, this->Distributor, "PKdTree");
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

  this->Superclass::ProcessViewInformation();
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::GatherInformation(vtkPVDataInformation* info)
{
  // When compositing is enabled, data is available at the update suppressors on
  // the render server (not the client), hence we change the server flag so that
  // the data is gathered from the correct server.
  if (this->GetUseCompositing())
    {
    this->UpdateSuppressor->SetServers(vtkProcessModule::RENDER_SERVER);
    }
  
  this->Superclass::GatherInformation(info);

  if (this->GetUseCompositing())
    {
    this->UpdateSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::GatherLODInformation(vtkPVDataInformation* info)
{
  // When compositing is enabled, data is available at the update suppressors on
  // the render server (not the client), hence we change the server flag so that
  // the data is gathered from the correct server.
  if (this->GetUseCompositing())
    {
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::RENDER_SERVER);
    }

  this->Superclass::GatherLODInformation(info);

  if (this->GetUseCompositing())
    {
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseOrderedCompositing: " << this->UseOrderedCompositing
    << endl;
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
}


