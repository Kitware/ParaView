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
vtkCxxRevisionMacro(vtkSMBlockDeliveryRepresentationProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy::vtkSMBlockDeliveryRepresentationProxy()
{
  this->Block = 0;
  this->BlockFilter = 0;
  this->Reduction = 0;
  this->CacheSize = 2;
  this->CleanCacheOnUpdate = true;
  this->Internal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkSMBlockDeliveryRepresentationProxy::~vtkSMBlockDeliveryRepresentationProxy()
{
  this->SetPostGatherHelper(0);
  delete this->Internal;
}

//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::IsCached(vtkIdType blockid)
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

  // For now, block filter always produces unstructured grid as the output.
  this->SetReductionType(CUSTOM);
  this->SetPostGatherHelper(this->Reduction);
  this->SetGenerateProcessIds(1);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Connect(input, this->BlockFilter, "Input", outputport);
  return this->Superclass::CreatePipeline(this->BlockFilter, 0);
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::MarkModified(vtkSMProxy* proxy)
{
  if (proxy !=  this)
    {
    this->CleanCacheOnUpdate = true;
    }

  this->Superclass::MarkModified(proxy);
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->UpdateRequired())
    {
    return;
    }

  if (this->CleanCacheOnUpdate)
    {
    this->CleanCache();
    this->CleanCacheOnUpdate = false;
    }

  if (this->Internal->CachedBlocks.find(this->Block)
    == this->Internal->CachedBlocks.end())
    {
    // Pass the block number to the BlockFilter.
    vtkSMIdTypeVectorProperty* ivp = vtkSMIdTypeVectorProperty::SafeDownCast(
      this->BlockFilter->GetProperty("Block"));
    if (ivp)
      {
      ivp->SetElement(0, this->Block);
      this->BlockFilter->UpdateProperty("Block", /*force=*/1);
      }
    }

  this->Superclass::Update(view);

  // Cache the data.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkAlgorithm* algorithm = 
    vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->StrategyProxy->GetOutput()->GetID()));

  vtkDataObject* output = vtkDataObject::SafeDownCast(
    algorithm->GetOutputDataObject(0));
  vtkDataObject* clone = output->NewInstance();
  clone->ShallowCopy(output);

  this->Internal->AddToCache(this->Block, clone, this->CacheSize);
  clone->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMBlockDeliveryRepresentationProxy::UpdateRequired()
{
  if (this->Internal->CachedBlocks.find(this->Block)
    == this->Internal->CachedBlocks.end())
    {
    return true;
    }

  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::CleanCache()
{
  this->Internal->CachedBlocks.clear();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSMBlockDeliveryRepresentationProxy::GetOutput()
{
 vtkInternal::CacheType::iterator iter = this->Internal->CachedBlocks.find(this->Block);
  if (iter != this->Internal->CachedBlocks.end())
    {
    iter->second.RecentUseTime.Modified();
    return iter->second.Dataobject.GetPointer();
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSMBlockDeliveryRepresentationProxy::GetBlockOutput()
{
  if (this->UpdateRequired())
    {
    this->Update(0);
    }

  return this->GetOutput();
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Block: " << this->Block << endl;
}


