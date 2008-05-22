/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredDataParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredDataParallelStrategy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIceTCompositeViewProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnstructuredDataParallelStrategy);
vtkCxxRevisionMacro(vtkSMUnstructuredDataParallelStrategy, "1.2");
//----------------------------------------------------------------------------
vtkSMUnstructuredDataParallelStrategy::vtkSMUnstructuredDataParallelStrategy()
{
  this->PreDistributorSuppressor = 0;
  this->Distributor = 0;

  this->PreDistributorSuppressorLOD = 0;
  this->DistributorLOD = 0;

  this->KdTree = 0;
  this->UseOrderedCompositing = false;
}

//----------------------------------------------------------------------------
vtkSMUnstructuredDataParallelStrategy::~vtkSMUnstructuredDataParallelStrategy()
{
  this->SetKdTree(0);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::ProcessViewInformation()
{
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
void vtkSMUnstructuredDataParallelStrategy::SetUseOrderedCompositing(bool use)
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
void vtkSMUnstructuredDataParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->PreDistributorSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressor"));
  this->Distributor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Distributor"));

  this->PreDistributorSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PreDistributorSuppressorLOD"));
  this->DistributorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("DistributorLOD"));

  this->PreDistributorSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Distributor->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->PreDistributorSuppressorLOD && this->DistributorLOD)
    {
    this->PreDistributorSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    this->DistributorLOD->SetServers(vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    this->SetEnableLOD(false);
    }
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->SetKdTree(this->KdTree);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::UpdateDistributedData()
{
  if (!this->DataValid)
    {
    // TODO: How to handle this when using cache?
    // We need to call this only if cache won't be used.
    this->PreDistributorSuppressor->InvokeCommand("ForceUpdate");
    // This is called for its side-effects; i.e. to force a PostUpdateData()
    this->PreDistributorSuppressor->UpdatePipeline();
    }
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);
  this->Connect(0, this->UpdateSuppressor);

  // Superclass will create such a pipeline
  //    INPUT --> Collect --> UpdateSuppressor
  // We want to insert the distrubutor between the collect and the update
  // suppressor.

  this->CreatePipelineInternal(this->Collect, 
                               this->PreDistributorSuppressor,
                               this->Distributor,
                               this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->Superclass::CreateLODPipeline(input, outputport);
  this->Connect(0, this->UpdateSuppressorLOD);
  // Superclass will create such a pipline
  //    INPUT --> LODDecimator --> Collect --> UpdateSuppressor
  // We want to insert the distrubutor between the collect and the update
  // suppressor.
  this->CreatePipelineInternal(this->CollectLOD, 
                               this->PreDistributorSuppressorLOD,
                               this->DistributorLOD,
                               this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* predistributorsuppressor,
  vtkSMSourceProxy* distributor,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(collect, predistributorsuppressor);

  // This sets the connection on the render server (since Distributor::Servers =
  // vtkProcessModule::RENDER_SERVER).
  this->Connect(predistributorsuppressor, distributor);

  // On Render Server, the Distributor is connected to the input of the
  // UpdateSuppressor. Since there are no distributors on the client
  // (or data server), we directly connect the PreDistributorSuppressor to the
  // UpdateSuppressor on the client and data server.
  this->Connect(predistributorsuppressor, updatesuppressor);
  updatesuppressor->UpdateVTKObjects();

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
void vtkSMUnstructuredDataParallelStrategy::SetKdTree(vtkSMProxy* proxy)
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
void vtkSMUnstructuredDataParallelStrategy::UpdatePipeline()
{
  bool usecompositing = this->GetUseCompositing();
  // cout << "usecompositing: " << usecompositing << endl;

  // cout << "use ordered compositing: " << (usecompositing && this->UseOrderedCompositing)
  //  << endl;
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Distributor->GetProperty("PassThrough"));
  ivp->SetElement(0,
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->Distributor->UpdateProperty("PassThrough");

  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::UpdateLODPipeline()
{
  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->GetUseCompositing();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DistributorLOD->GetProperty("PassThrough"));
  ivp->SetElement(0,
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->DistributorLOD->UpdateProperty("PassThrough");

  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseOrderedCompositing: " 
    << this->UseOrderedCompositing << endl;
}


