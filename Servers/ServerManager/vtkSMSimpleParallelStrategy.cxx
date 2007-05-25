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
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
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
vtkCxxRevisionMacro(vtkSMSimpleParallelStrategy, "1.1");
//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::vtkSMSimpleParallelStrategy()
{
  this->Collect = 0;
  this->PreDistributorSuppressor = 0;
  this->Distributor = 0;

  this->CollectLOD = 0;
  this->PreDistributorSuppressorLOD = 0;
  this->DistributorLOD = 0;
}

//----------------------------------------------------------------------------
vtkSMSimpleParallelStrategy::~vtkSMSimpleParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

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

  this->CollectLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->PreDistributorSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->DistributorLOD->SetServers(vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);
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
void vtkSMSimpleParallelStrategy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->CreatePipelineInternal(input,
                               this->Collect, 
                               this->PreDistributorSuppressor,
                               this->Distributor,
                               this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->LODDecimator);
  this->CreatePipelineInternal(this->LODDecimator,
                               this->CollectLOD, 
                               this->PreDistributorSuppressorLOD,
                               this->DistributorLOD,
                               this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* input,
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* predistributorsuppressor,
  vtkSMSourceProxy* distributor,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(input, collect);
  this->Connect(collect, predistributorsuppressor);

  // This sets the connection on the render server (since Distributor::Servers =
  // vtkProcessModule::RENDER_SERVER).
  this->Connect(predistributorsuppressor, distributor);

  // On Render Server, the Distributor is connected to the input of the
  // UpdateSuppressor. Since there are not distributors on the client
  // (or data server), we directly connect the PreDistributorSuppressor to the
  // UpdateSuppressor on the client and data server.
  stream  << vtkClientServerStream::Invoke
          << distributor->GetID(0) 
          << "GetOutputPort" << 0
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << updatesuppressor->GetID(0)
          << "SetInputConnection" << 0 
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);

  // Now send to the dataserver/client
  stream  << vtkClientServerStream::Invoke
          << predistributorsuppressor->GetID(0)
          << "GetOutputPort" << 0
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << updatesuppressor->GetID(0)
          << "SetInputConnection" << 0
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);

  // Now we need to set up some default parameters on these filters.

  // Collect filter needs the socket controller use to communicate between
  // data-server root and the client.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetSocketController"
          << pm->GetConnectionClientServerID(this->ConnectionID)
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << collect->GetID(0)
          << "SetSocketController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER_ROOT, stream);

  // Collect filter needs the MPIMToNSocketConnection to communicate between
  // render server and data server nodes.
  stream  << vtkClientServerStream::Invoke
          << collect->GetID(0)
          << "SetMPIMToNSocketConnection"
          << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  // Set the server flag on the collect filter to correctly identify each
  // processes.
  stream  << vtkClientServerStream::Invoke
          << collect->GetID(0)
          << "SetServerToRenderServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collect->GetID(0)
          << "SetServerToDataServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::DATA_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collect->GetID(0)
          << "SetServerToClient"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
  

  // Set the MultiProcessController on the distributor.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << distributor->GetID(0)
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
bool vtkSMSimpleParallelStrategy::UseCompositing()
{
  return (this->ViewHelperProxy && 
    vtkSMSimpleParallelStrategyGetInt(this->ViewHelperProxy, "UseCompositing", 0));
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdatePipeline()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->UseCompositing();
  cout << "usecompositing: " << usecompositing << endl;

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("MoveMode"));
  ivp->SetElement(0, 
    usecompositing? vtkMPIMoveData::PASS_THROUGH : vtkMPIMoveData::COLLECT);
  this->Collect->UpdateProperty("MoveMode");

  cout << "use ordered compositing: " << (usecompositing && this->UseOrderedCompositing)
    << endl;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Distributor->GetProperty("PassThrough"));
  ivp->SetElement(0,
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->Distributor->UpdateProperty("PassThrough");

  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::UpdateLODPipeline()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->UseCompositing();

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

  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::SetUseOrderedCompositing(bool use)
{
  if (this->UseOrderedCompositing != use)
    {
    this->UseOrderedCompositing = use;

    // invalidate data, since distribution changed.
    this->MarkModified(0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseOrderedCompositing: " << this->UseOrderedCompositing
    << endl;
}


