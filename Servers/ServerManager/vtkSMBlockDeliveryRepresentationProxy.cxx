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
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"

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
vtkCxxRevisionMacro(vtkSMBlockDeliveryRepresentationProxy, "1.7");
//----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy::vtkSMBlockDeliveryRepresentationProxy()
{
  this->BlockFilter = 0;
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
void vtkSMBlockDeliveryRepresentationProxy::SetFieldType(int ft)
{
  if (this->BlockFilter)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->BlockFilter->GetProperty("FieldType"));
    if (ivp)
      {
      ivp->SetElement(0, ft);
      this->BlockFilter->UpdateProperty("FieldType");
      this->CacheDirty = true;
      }
    }
}


//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::SetProcessID(int id)
{
  if (this->BlockFilter)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->BlockFilter->GetProperty("ProcessID"));
    if (ivp)
      {
      ivp->SetElement(0, id);
      this->BlockFilter->UpdateProperty("ProcessID");
      this->CacheDirty = true;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::SetCompositeDataSetIndex(int id)
{
  if (id < 0)
    {
    id = 0;
    }
  if (this->BlockFilter)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->BlockFilter->GetProperty("CompositeDataSetIndex"));
    if (ivp)
      {
      ivp->SetElement(0, id);
      this->BlockFilter->UpdateProperty("CompositeDataSetIndex");
      this->CacheDirty = true;
      }
    }
  this->CompositeDataSetIndex = static_cast<unsigned int>(id);
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

  // Block filter is used to deliver only 1 block of data to the client.
  this->BlockFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("BlockFilter"));
  this->BlockFilter->SetServers(vtkProcessModule::DATA_SERVER);

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
  this->Connect(input, this->UpdateStrategy, "Input", outputport);
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

  this->Connect(this->UpdateStrategy->GetOutput(), this->BlockFilter);
  this->Connect(this->BlockFilter, this->DeliveryStrategy);

  // Set default strategy values.
  this->DeliveryStrategy->SetPreGatherHelper((vtkSMProxy*)0);
  this->DeliveryStrategy->SetPostGatherHelper(this->Reduction);
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DeliveryStrategy->GetProperty("GenerateProcessIds"));
  ivp->SetElement(0, 1);
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
}

//----------------------------------------------------------------------------
// Ensure that the block selected by \c block is available on the client.
void vtkSMBlockDeliveryRepresentationProxy::Fetch(vtkIdType block)
{
  vtkInternal::CacheType::iterator iter = 
    this->Internal->CachedBlocks.find(block);
  if (iter == this->Internal->CachedBlocks.end())
    {
    // cout << this << " Fetching Block #" << block << endl;
    // Pass the block number to the BlockFilter.
    vtkSMIdTypeVectorProperty* ivp = vtkSMIdTypeVectorProperty::SafeDownCast(
      this->BlockFilter->GetProperty("Block"));
    if (ivp)
      {
      ivp->SetElement(0, block);
      this->BlockFilter->UpdateProperty("Block");
      }
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
void vtkSMBlockDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << endl;
}


