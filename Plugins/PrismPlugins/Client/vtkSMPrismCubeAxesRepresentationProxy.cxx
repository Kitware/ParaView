/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismCubeAxesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPrismCubeAxesRepresentationProxy.h"

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
#include "vtkSMIntVectorProperty.h"

inline void vtkSMPrismRepresentationProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    }
  proxy->UpdateProperty(pname);
}


vtkStandardNewMacro(vtkSMPrismCubeAxesRepresentationProxy);
vtkCxxRevisionMacro(vtkSMPrismCubeAxesRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::vtkSMPrismCubeAxesRepresentationProxy()
{
  this->OutlineFilter = 0;
  this->CubeAxesProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::~vtkSMPrismCubeAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->OutlineFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("OutlineFilter"));
  this->CubeAxesProxy = this->GetSubProxy("Prop2D");
  this->Property = this->GetSubProxy("Property");

  if (!this->OutlineFilter || !this->CubeAxesProxy || !this->Property)
    {
    vtkErrorMacro("Missing required subproxies");
    return false;
    }

  this->OutlineFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->CubeAxesProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  this->Connect(input, this->OutlineFilter);
  this->Connect(this->Property, this->CubeAxesProxy, "Property");
  
  vtkSMPrismRepresentationProxySetInt(this->CubeAxesProxy, "Visibility", 0);

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::InitializeStrategy(
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
bool vtkSMPrismCubeAxesRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  renderView->AddPropToRenderer(this->CubeAxesProxy);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << renderView->GetRendererProxy()->GetID()
          << "GetActiveCamera"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesProxy->GetID()
          << "SetCamera"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPrismCubeAxesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->CubeAxesProxy);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->CubeAxesProxy->GetID()
          << "SetCamera" << 0
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);
  this->Strategy = 0;
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
void vtkSMPrismCubeAxesRepresentationProxy::Update(vtkSMViewProxy* view)
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
        this->CubeAxesProxy->GetProperty("Bounds"));
      dvp->SetElements(info->GetBounds());

      vtkSMSourceProxy* input = this->GetInputProxy();
      vtkSMProperty *rangeProperty = input->GetProperty("RangesInfo");
      input->UpdatePropertyInformation(rangeProperty);
       vtkSMDoubleVectorProperty* rangeVP = vtkSMDoubleVectorProperty::SafeDownCast(
        rangeProperty);

  
       vtkSMDoubleVectorProperty* newRangeVP = vtkSMDoubleVectorProperty::SafeDownCast(
        this->CubeAxesProxy->GetProperty("Ranges"));

      newRangeVP->SetElement(0,rangeVP->GetElement(0));
      newRangeVP->SetElement(1,rangeVP->GetElement(1));
      newRangeVP->SetElement(2,rangeVP->GetElement(2));
      newRangeVP->SetElement(3,rangeVP->GetElement(3));
      newRangeVP->SetElement(4,rangeVP->GetElement(4));
      newRangeVP->SetElement(5,rangeVP->GetElement(5));


  
      vtkSMPrismRepresentationProxySetInt(this->CubeAxesProxy, "UseRanges",
   1);



      this->CubeAxesProxy->UpdateVTKObjects();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMPrismCubeAxesRepresentationProxy::SetCubeAxesVisibility(int visible)
{
  this->CubeAxesVisibility = visible;
  vtkSMPrismRepresentationProxySetInt(this->CubeAxesProxy, "Visibility",
    visible);
  this->CubeAxesProxy->UpdateVTKObjects();
}


//----------------------------------------------------------------------------
void vtkSMPrismCubeAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


