/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCubeAxesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCubeAxesRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMCubeAxesRepresentationProxy);
vtkCxxRevisionMacro(vtkSMCubeAxesRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMCubeAxesRepresentationProxy::vtkSMCubeAxesRepresentationProxy()
{
  this->OutlineFilter = 0;
  this->CubeAxesActor = 0;
}

//----------------------------------------------------------------------------
vtkSMCubeAxesRepresentationProxy::~vtkSMCubeAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->OutlineFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("OutlineFilter"));
  this->CubeAxesActor = this->GetSubProxy("Prop2D");

  if (!this->OutlineFilter || !this->CubeAxesActor)
    {
    vtkErrorMacro("Missing required subproxies");
    return false;
    }

  this->OutlineFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->CubeAxesActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  this->Connect(input, this->OutlineFilter);
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::InitializeStrategy(
  vtkSMViewProxy* vtkNotUsed(view))
{
  // Since we use an outline filter, the data type fed into the strategy is
  // always polydata.
  // Also, we don't need to deliver the data anywhere, since we will obtain
  // bounds using vtkPVDataInformation. Hence we directly create the
  // BlockDeliveryStrategy. We use BlockDeliveryStrategy since it does not
  // create any update suppressor on render server or client -- only the data
  // server where the data is.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  // Initialize strategy for the selection pipeline.
  strategy.TakeReference(vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "BlockDeliveryStrategy")));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("Could not create strategy for selection pipeline.");
    return false;
    }

  strategy->SetConnectionID(this->ConnectionID);

  // LOD pipeline not needed.
  strategy->SetEnableLOD(false);
  // Caching may be employed when requested.
  strategy->SetEnableCaching(true);

  this->Connect(this->OutlineFilter, strategy);

  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);
  this->Strategy = strategy; // we keep a reference to make it easy to access the output.
  return true;
}
//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  renderView->AddPropToRenderer(this->CubeAxesActor);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << renderView->GetRendererProxy()->GetID()
          << "GetActiveCamera"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMCubeAxesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->CubeAxesActor);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesActor->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  this->Strategy = 0;
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
void vtkSMCubeAxesRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  if (this->GetVisibility() && this->Strategy)
    {
    // Get bounds and set on the actor.
    vtkSMSourceProxy* output = this->Strategy->GetOutput();
    vtkPVDataInformation* info = output->GetDataInformation();
    if (info)
      {
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->CubeAxesActor->GetProperty("Bounds"));
      dvp->SetElements(info->GetBounds());
      this->CubeAxesActor->UpdateVTKObjects();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMCubeAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


