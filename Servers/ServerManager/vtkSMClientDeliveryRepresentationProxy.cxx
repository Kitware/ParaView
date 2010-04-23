/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientDeliveryRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientDeliveryRepresentationProxy.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMClientDeliveryRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::vtkSMClientDeliveryRepresentationProxy()
{
  this->PreProcessorProxy = 0;
  this->StrategyProxy = 0;
  this->PostProcessorProxy = 0;

  this->ReductionType = 0;

  this->PreGatherHelper = 0;
  this->PostGatherHelper = 0;
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::~vtkSMClientDeliveryRepresentationProxy()
{
  if (this->StrategyProxy)
    {
    this->StrategyProxy->Delete();
    }
  this->StrategyProxy = 0;
  this->PreProcessorProxy = 0;
  this->PostProcessorProxy = 0;
  if (this->PreGatherHelper)
    {
    this->PreGatherHelper->Delete();
    }
  if (this->PostGatherHelper)
    {    
    this->PostGatherHelper->Delete();
    }

}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::SetupStrategy(vtkSMSourceProxy* input,
  int outputport)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  this->StrategyProxy = vtkSMClientDeliveryStrategyProxy::SafeDownCast(
    pxm->NewProxy("strategies", "ClientDeliveryStrategy"));
  if (!this->StrategyProxy)
    {
    vtkErrorMacro("Failed to create vtkSMClientDeliveryStrategyProxy.");
    return false;
    }

  this->StrategyProxy->SetConnectionID(this->ConnectionID);

  this->AddStrategy(this->StrategyProxy);

  this->StrategyProxy->SetEnableLOD(false);

  // Creates the strategy objects.
  this->StrategyProxy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  this->Connect(input, this->StrategyProxy, 
    "Input", outputport);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->PreProcessorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("PreProcessor"));
  if (this->PreProcessorProxy)
    {
    this->PreProcessorProxy->SetServers(
      vtkProcessModule::DATA_SERVER);
    }

  this->PostProcessorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("PostProcessor"));
  if (this->PostProcessorProxy)
    {
    this->PostProcessorProxy->SetServers(vtkProcessModule::CLIENT);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::EndCreateVTKObjects()
{
  // Setup selection pipeline connections.
  vtkSMSourceProxy* input = this->GetInputProxy();
  int port = this->OutputPort;
  if (this->PreProcessorProxy)
    {
    this->Connect(input, this->PreProcessorProxy, "Input", port);
    input = this->PreProcessorProxy;
    port = 0;
    }
  this->CreatePipeline(input, port);
  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::CreatePipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->SetupStrategy(input, outputport);

  if (this->PostProcessorProxy)
    {
    this->Connect(this->StrategyProxy->GetOutput(), this->PostProcessorProxy);
    this->PostProcessorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetReductionType(int type)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot set reduction type before CreateVTKObjects().");
    return;
    }

  if(this->ReductionType == type)
    {
    return;
    }

  this->ReductionType = type;

  const char* classname = 0;
  switch (type)
    {
  case ADD:
    classname = "vtkAttributeDataReductionFilter";
    break;

  case POLYDATA_APPEND:
    classname = "vtkAppendPolyData";
    break;

  case UNSTRUCTURED_APPEND:
    classname = "vtkAppendFilter";
    break;

  case FIRST_NODE_ONLY:
    classname = 0;
    break;

  case RECTILINEAR_GRID_APPEND:
    classname = "vtkAppendRectilinearGrid";
    break;

  case COMPOSITE_DATASET_APPEND:
    classname = "vtkMultiBlockDataGroupFilter"; 
    break;

  case CUSTOM:
    this->StrategyProxy->SetPreGatherHelper(this->PreGatherHelper);
    this->StrategyProxy->SetPostGatherHelper(this->PostGatherHelper);
    return;

  case MULTIBLOCK_MERGE:
    classname = "vtkMultiBlockMergeFilter"; 
    break;
  
  case TABLE_MERGE:
    classname = "vtkPVMergeTables";
    break;
  
  default:
    vtkErrorMacro("Unknown reduction type: " << type);
    return;
    }

  // this method remove pre/post helper proxies set when mode is CUSTOM.
  this->StrategyProxy->SetPostGatherHelper(classname);
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetPostGatherHelper(vtkSMProxy* proxy)
{
  vtkSetObjectBodyMacro(PostGatherHelper, vtkSMProxy, proxy);

  if (this->ReductionType == CUSTOM)
    {
    this->StrategyProxy->SetPostGatherHelper(proxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetPreGatherHelper(vtkSMProxy* proxy)
{
  vtkSetObjectBodyMacro(PreGatherHelper, vtkSMProxy, proxy);

  if (this->ReductionType == CUSTOM)
    {
    this->StrategyProxy->SetPreGatherHelper(proxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetPassThrough(int pt)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->StrategyProxy->GetProperty("PassThrough"));
  if (ivp)
    {
    ivp->SetElement(0, pt);
    this->StrategyProxy->UpdateProperty("PassThrough");
    }
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetGenerateProcessIds(int pt)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->StrategyProxy->GetProperty("GenerateProcessIds"));
  if (ivp)
    {
    ivp->SetElement(0, pt);
    this->StrategyProxy->UpdateProperty("GenerateProcessIds");
    }
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkSMClientDeliveryRepresentationProxy::GetOutput()
{
  vtkAlgorithm* dp = 0;
  if (this->PostProcessorProxy)
    {
    dp = vtkAlgorithm::SafeDownCast(
      this->PostProcessorProxy->GetClientSideObject()); 
    }
  else
    {
    if (this->StrategyProxy && this->StrategyProxy->GetOutput())
      {
      dp = vtkAlgorithm::SafeDownCast(
        this->StrategyProxy->GetOutput()->GetClientSideObject());
      }
    }

  return dp? dp->GetOutputDataObject(0) : NULL;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSMClientDeliveryRepresentationProxy::GetOutputPort()
{
  vtkAlgorithm* dp = 0;
  if (this->PostProcessorProxy)
    {
    dp = vtkAlgorithm::SafeDownCast(
      this->PostProcessorProxy->GetClientSideObject()); 
    }
  else
    {
    if (this->StrategyProxy && this->StrategyProxy->GetOutput())
      {
      dp = vtkAlgorithm::SafeDownCast(
        this->StrategyProxy->GetOutput()->GetClientSideObject());
      }
    }

  return dp? dp->GetOutputPort(0) : NULL;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  this->Superclass::Update(view);

  if (this->PostProcessorProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID())); 
    if (!dp)
      {
      vtkErrorMacro("Failed to get algorithm for PostProcessorProxy.");
      }
    else
      {
      dp->Update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


