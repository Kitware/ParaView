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
#include "vtkSMIceTCompositeViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkSMUnstructuredDataParallelStrategy);
//----------------------------------------------------------------------------
vtkSMUnstructuredDataParallelStrategy::vtkSMUnstructuredDataParallelStrategy()
{
  this->Distributor = 0;
  this->PostDistributorSuppressor = 0;

  this->DistributorLOD = 0;
  this->PostDistributorSuppressorLOD = 0;

  this->DistributedDataValid = false;
  this->DistributedLODDataValid = false;

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
    this->DistributedDataValid = false;
    this->DistributedLODDataValid = false;
    }
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->PostDistributorSuppressor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PostDistributorSuppressor"));
  this->Distributor =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Distributor"));

  this->PostDistributorSuppressorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PostDistributorSuppressorLOD"));
  this->DistributorLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("DistributorLOD"));

  this->PostDistributorSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->Distributor->SetServers(vtkProcessModule::RENDER_SERVER);

  if (this->PostDistributorSuppressorLOD && this->DistributorLOD)
    {
    this->PostDistributorSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
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
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::UpdateDistributedData()
{
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::InvalidateDistributedData()
{
  this->DistributedDataValid = false;
  this->DistributedLODDataValid = false;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  // Extend the superclass's pipeline as follows
  // SUPERCLASS --> Distributor (only of Render Server) --> PostDistributorSuppressor
  this->CreatePipelineInternal(this->Superclass::GetOutput(),
                               this->Distributor,
                               this->PostDistributorSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->Superclass::CreateLODPipeline(input, outputport);

  // Extend the superclass's pipeline as follows
  // SUPERCLASS --> DistributorLOD (only of Render Server) -->
  //                                            PostDistributorSuppressorLOD
  this->CreatePipelineInternal(this->Superclass::GetLODOutput(),
                               this->DistributorLOD,
                               this->PostDistributorSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::CreatePipelineInternal(
  vtkSMSourceProxy* input,
  vtkSMSourceProxy* distributor,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // This sets the connection on the render server (since Distributor::Servers =
  // vtkProcessModule::RENDER_SERVER).
  this->Connect(input, distributor);

  // On Render Server, the Distributor is connected to the input of the
  // UpdateSuppressor. Since there are no distributors on the client
  // (or data server), we directly connect the PostDistributorSuppressor to the
  // UpdateSuppressor on the client and data server.
  this->Connect(input, updatesuppressor);
  updatesuppressor->UpdateVTKObjects();

  // Connect the distrubutor and set it up, only on the render server.
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
  
  // The distributor does not do any distribution by default.
  vtkSMPropertyHelper(distributor, "PassThrough").Set(1);
  distributor->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::SetKdTree(vtkSMProxy* proxy)
{
  if (this->KdTree != proxy)
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
  //this->DistributedDataValid = false;
  //this->DistributedLODDataValid = false;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::UpdatePipeline()
{
  if (this->vtkSMUnstructuredDataParallelStrategy::GetDataValid())
    {
    return;
    }

  this->Superclass::UpdatePipeline();

  bool usecompositing = this->GetUseCompositing();
  // cout << "usecompositing: " << usecompositing << endl;

  // cout << "use ordered compositing: " << (usecompositing && this->UseOrderedCompositing)
  //  << endl;
  vtkSMPropertyHelper(this->Distributor, "PassThrough").Set(
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->Distributor->UpdateProperty("PassThrough");

  this->PostDistributorSuppressor->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->PostDistributorSuppressor->UpdatePipeline();
  this->DistributedDataValid = true;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::UpdateLODPipeline()
{
  if (this->vtkSMUnstructuredDataParallelStrategy::GetLODDataValid())
    {
    return;
    }

  this->Superclass::UpdateLODPipeline();

  // Based on the compositing decision made by the render view,
  // decide where the data should be delivered for rendering.

  bool usecompositing = this->GetUseCompositing();

  vtkSMPropertyHelper(this->DistributorLOD, "PassThrough").Set(
    (usecompositing && this->UseOrderedCompositing)? 0 : 1);
  this->DistributorLOD->UpdateProperty("PassThrough");

  this->PostDistributorSuppressorLOD->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->PostDistributorSuppressorLOD->UpdatePipeline();
  this->DistributedLODDataValid = true;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredDataParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseOrderedCompositing: " 
    << this->UseOrderedCompositing << endl;
}


