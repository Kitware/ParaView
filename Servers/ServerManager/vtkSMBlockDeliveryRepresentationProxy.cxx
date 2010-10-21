/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBlockDeliveryRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBlockDeliveryRepresentationProxy.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <vtkstd/map>

//----------------------------------------------------------------------------
class vtkSMBlockDeliveryRepresentationProxy::vtkInternal
{
public:
  class CacheInfo
    {
  public:
    vtkSmartPointer<vtkDataObject> Dataobject;
    vtkTimeStamp RecentUseTime;
    };

  typedef vtkstd::map<vtkIdType, CacheInfo> CacheType;
  CacheType CachedBlocks;

  void AddToCache(vtkIdType blockId, vtkDataObject* data, vtkIdType max)
    {
    CacheType::iterator iter = this->CachedBlocks.find(blockId);
    if (iter != this->CachedBlocks.end())
      {
      this->CachedBlocks.erase(iter);
      }

    if (static_cast<vtkIdType>(this->CachedBlocks.size()) == max)
      {
      // remove least-recent-used block.
      iter = this->CachedBlocks.begin();
      CacheType::iterator iterToRemove = this->CachedBlocks.begin();
      for (; iter != this->CachedBlocks.end(); ++iter)
        {
        if (iterToRemove->second.RecentUseTime > iter->second.RecentUseTime)
          {
          iterToRemove = iter;
          }
        }
      this->CachedBlocks.erase(iterToRemove);
      }

    vtkInternal::CacheInfo info;
    info.Dataobject = data;
    info.RecentUseTime.Modified();
    this->CachedBlocks[blockId] = info;
    }
};

vtkStandardNewMacro(vtkSMBlockDeliveryRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy::vtkSMBlockDeliveryRepresentationProxy()
{
  this->PreProcessor = 0;
  this->Streamer = 0;
  this->CacheDirty = false;
  this->UpdateStrategy = 0;
  this->DeliveryStrategy = 0;
  this->Reduction = 0;
  this->CacheSize = 2;
  this->CompositeDataSetIndex = 0;
  this->Internal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy::~vtkSMBlockDeliveryRepresentationProxy()
{
  if (this->DeliveryStrategy)
    {
    this->DeliveryStrategy->SetPostGatherHelper((vtkSMProxy*)0);
    this->DeliveryStrategy->Delete();
    this->DeliveryStrategy = 0;
    }
  this->UpdateStrategy = 0;
  delete this->Internal;
}

//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::IsAvailable(vtkIdType blockid)
{
  return this->Internal->CachedBlocks.find(blockid) != 
    this->Internal->CachedBlocks.end();
}

//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->PreProcessor = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("PreProcessor"));
  if (this->PreProcessor)
    {
    this->PreProcessor->SetServers(vtkProcessModule::DATA_SERVER);
    }

  // Block filter is used to deliver only 1 block of data to the client.
  this->Streamer = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Streamer"));
  this->Streamer->SetServers(vtkProcessModule::DATA_SERVER);

  this->Reduction = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Reduction"));
  this->Reduction->SetServers(vtkProcessModule::DATA_SERVER);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  if (!this->CreatePipeline(this->GetInputProxy(), this->OutputPort))
    {
    return false;
    }

  return true;
}


//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Block delivery uses two strategies
  // * to update the pipeline.
  // * to deliver chunk of data.

  // Create the strategy use to update the representation.
  this->UpdateStrategy = vtkSMRepresentationStrategy::SafeDownCast(
    pxm->NewProxy("strategies", "BlockDeliveryStrategy"));
  if (!this->UpdateStrategy)
    {
    return false;
    }
  this->UpdateStrategy->SetConnectionID(this->ConnectionID);
  this->AddStrategy(this->UpdateStrategy);
  this->UpdateStrategy->Delete();

  this->UpdateStrategy->SetEnableLOD(false);
  if (this->PreProcessor)
    {
    this->Connect(input, this->PreProcessor, "Input", outputport);
    this->Connect(this->PreProcessor, this->UpdateStrategy);
    }
  else
    {
    this->Connect(input, this->UpdateStrategy, "Input", outputport);
    }
  this->UpdateStrategy->UpdateVTKObjects();

  // Now create another strategy to deliver the data to the client.
  // This is an internal strategy i.e. it is not dependent on Update() calls
  // done by the view, the representation is free to update this strategy
  // whenever it feels suitable.
  this->DeliveryStrategy = vtkSMClientDeliveryStrategyProxy::SafeDownCast(
    pxm->NewProxy("strategies", "ClientDeliveryStrategy"));
  if (!this->DeliveryStrategy)
    {
    return false;
    }
  this->DeliveryStrategy->SetConnectionID(this->ConnectionID);
  this->DeliveryStrategy->SetEnableLOD(false);

  this->Connect(this->UpdateStrategy->GetOutput(), this->Streamer);
  this->Connect(this->Streamer, this->DeliveryStrategy);

  // Set default strategy values.
  this->DeliveryStrategy->SetPreGatherHelper((vtkSMProxy*)0);
  this->DeliveryStrategy->SetPostGatherHelper(this->Reduction);
  vtkSMPropertyHelper(this->DeliveryStrategy, "GenerateProcessIds").Set(1);
  this->DeliveryStrategy->UpdateVTKObjects();;
  return true;
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->UpdateRequired() || this->CacheDirty)
    {
    // Our cache becomes obsolete following this update.
    this->CleanCache();
    this->CacheDirty = false;
    }

  this->Superclass::Update(view);
  // HACK: Please fix the domains soon so I won't need to do this stupidity.
  if (this->PreProcessor &&
    this->PreProcessor->GetProperty("Input"))
    {
    this->PreProcessor->GetProperty("Input")->UpdateDependentDomains();
    }
}

//----------------------------------------------------------------------------
// Ensure that the block selected by \c block is available on the client.
void vtkSMBlockDeliveryRepresentationProxy::Fetch(vtkIdType block)
{
  // Pass the block number to the Streamer. (needed for the selection)
  vtkSMIdTypeVectorProperty* ivp = vtkSMIdTypeVectorProperty::SafeDownCast(
    this->Streamer->GetProperty("Block"));
  if (ivp)
    {
    ivp->SetElement(0, block);
    this->Streamer->UpdateProperty("Block");
    }

  // Look in the cache if computation should be done or not
  vtkInternal::CacheType::iterator iter = 
    this->Internal->CachedBlocks.find(block);
  if (iter == this->Internal->CachedBlocks.end())
    {
    // cout << this << " Fetching Block #" << block << endl;
    this->DeliveryStrategy->Update();

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAlgorithm* algorithm = 
      vtkAlgorithm::SafeDownCast(
        pm->GetObjectFromID(this->DeliveryStrategy->GetOutput()->GetID()));

    vtkDataObject* output = vtkDataObject::SafeDownCast(
      algorithm->GetOutputDataObject(0));

    vtkDataObject* clone = output->NewInstance();
    clone->ShallowCopy(output);
    this->Internal->AddToCache(block, clone, this->CacheSize);
    this->IsAvailable(block);
    clone->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::CleanCache()
{
  this->Internal->CachedBlocks.clear();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSMBlockDeliveryRepresentationProxy::GetOutput(vtkIdType block)
{
  // Ensure that the current block is available on client.
  this->Fetch(block);

  vtkInternal::CacheType::iterator iter = 
    this->Internal->CachedBlocks.find(block);
  if (iter != this->Internal->CachedBlocks.end())
    {
    iter->second.RecentUseTime.Modified();
    return iter->second.Dataobject.GetPointer();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkSMBlockDeliveryRepresentationProxy::GetNumberOfRequiredBlocks()
{
  vtkPVDataInformation* dInfo = this->GetRepresentedDataInformation(true);
  return static_cast<vtkIdType>(ceil(
      static_cast<double>(dInfo->GetNumberOfRows())/
      vtkSMPropertyHelper(this, "BlockSize").GetAsIdType()));
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << endl;
}


