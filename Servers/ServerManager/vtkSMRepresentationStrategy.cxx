/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentationStrategy.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataSizeInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPropertyHelper.h"

vtkCxxSetObjectMacro(vtkSMRepresentationStrategy, 
  RepresentedDataInformation, vtkPVDataInformation);
//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::vtkSMRepresentationStrategy()
{
  this->UseCache = false;
  this->UseLOD = false;
  this->Input = 0;
  this->ViewInformation = 0;
  this->EnableLOD = false;
  this->EnableCaching = true;
 
  this->LODDataValid = false;
  this->LODDataSize = 0;
  this->LODResolution = 50;
  this->LODInformationValid =false;

  this->DataValid = false;
  this->DataSize = 0;
  this->InformationValid = false;

  vtkMemberFunctionCommand<vtkSMRepresentationStrategy>* command =
    vtkMemberFunctionCommand<vtkSMRepresentationStrategy>::New();
  command->SetCallback(*this,
    &vtkSMRepresentationStrategy::ProcessViewInformation);
  this->Observer = command;

  this->KeepLODPipelineUpdated = false;
  this->RepresentedDataInformation = 0;
  vtkPVGeometryInformation* info = vtkPVGeometryInformation::New();
  this->SetRepresentedDataInformation(info);
  info->Delete();

  this->CacheKeeper = 0;
  this->SomethingCached = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::~vtkSMRepresentationStrategy()
{
  this->AddInput(0, 0, 0, 0);
  this->SetViewInformation(0);

  this->Observer->Delete();
  this->Observer = 0;

  this->SetRepresentedDataInformation(0);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::SetViewInformation(vtkInformation* info)
{
  if (this->ViewInformation)
    {
    this->ViewInformation->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(ViewInformation, vtkInformation, info);

  if (info)
    {
    this->ViewInformation->AddObserver(vtkCommand::ModifiedEvent,
      this->Observer);

    if (this->ObjectsCreated)
      {
      // Don't call ProcessViewInformation() unless we know that the objects
      // have been created. ProcessViewInformation() is called in
      // CreateVTKObjects() as well.
      // Get the current values from the view helper.
      this->ProcessViewInformation();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  // If properties on the strategy itself are modified, then we are assuming
  // that the pipeline inside the strategy will get affected.
  // This is generally the case since a strategy does not include any
  // mappers/actors but pipelines upto a mapper. Hence this assumption is
  // generally valid.

  // Mark all data invalid.
  // Note that we are not marking the information invalid. The information
  // won't get invalidated until the pipeline is updated.
  this->InvalidatePipeline();
  this->InvalidateLODPipeline();
  
  this->Superclass::MarkDirty(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::InvalidatePipeline()
{
  this->DataValid = false;
  this->InformationValid = false;
  this->CleanCacheIfObsolete();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::InvalidateLODPipeline()
{
  this->LODDataValid = false;
  this->LODInformationValid = false;
  this->CleanCacheIfObsolete();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::CleanCacheIfObsolete()
{
  if (this->SomethingCached && !this->GetUseCache())
    {
    this->SomethingCached = false;
    this->CacheKeeper->InvokeCommand("RemoveAllCaches");
    }
}

//----------------------------------------------------------------------------
unsigned long vtkSMRepresentationStrategy::GetDisplayedMemorySize()
{
  if (this->GetUseLOD())
    {
    return this->GetLODMemorySize();
    }
  return this->GetFullResMemorySize();
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::GetUseLOD()
{
  return (this->EnableLOD && this->UseLOD);
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::GetUseCache()
{
  return (this->EnableCaching && this->UseCache);
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::UpdateRequired()
{
  // Since fullres is pipeline always needs to be up-to-date, if it is not
  // up-to-date, we need an Update. Additionally, is LOD is enabled and LOD
  // pipeline is not up-to-date, then too, we need an update.

  bool update_required = !this->GetDataValid();

  if (this->GetUseLOD() || (this->EnableLOD && this->KeepLODPipelineUpdated))
    {
    update_required |= !this->GetLODDataValid();
    }

  return update_required;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::Update()
{
  if (this->UpdateRequired())
    {
    this->InvokeEvent(vtkCommand::StartEvent);
 
    if (!this->GetDataValid())
      {
      this->UpdatePipeline();
      }

    if ((this->GetUseLOD() || (this->EnableLOD && this->KeepLODPipelineUpdated)) 
      && !this->GetLODDataValid())
      {
      this->UpdateLODPipeline();
      }

    this->InvokeEvent(vtkCommand::EndEvent);
    }

  this->PostUpdateData();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::UpdateDataInformation()
{
  if (!this->InformationValid)
    {
    vtkPVDataInformation* info = vtkPVGeometryInformation::New();
    this->GatherInformation(info);
    this->SetRepresentedDataInformation(info);
    this->DataSize = static_cast<unsigned long>(info->GetMemorySize());
    this->InformationValid = true;
    info->Delete();
    }

  if ( (this->GetUseLOD() || (this->EnableLOD && this->KeepLODPipelineUpdated)) 
    && !this->LODInformationValid)
    {
    vtkPVDataSizeInformation* info = vtkPVDataSizeInformation::New();
    this->GatherLODInformation(info);
    this->LODDataSize = static_cast<unsigned long>(info->GetMemorySize());
    this->LODInformationValid = true;
    info->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::AddInput(unsigned int vtkNotUsed(inputPort),
                                           vtkSMSourceProxy* input,
                                           unsigned int outputPort,
                                           const char* vtkNotUsed(method))
{
  vtkSetObjectBodyMacro(Input, vtkSMSourceProxy, input);
  this->OutputPort = outputPort;
  if (!this->Input)
    {
    return;
    }

  // Not using the input number of parts here since that logic
  // is going to disappear in near future.
  this->CreateVTKObjects();

  this->Connect(this->Input, this->CacheKeeper, "Input", this->OutputPort);
  this->CreatePipeline(this->CacheKeeper, 0);

  // LOD pipeline is created only if EnableLOD is true.
  if (this->EnableLOD)
    {
    this->CreateLODPipeline(this->CacheKeeper, 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::CreateVTKObjects()
{
  if (!this->ObjectsCreated)
    {
    this->BeginCreateVTKObjects();
    this->Superclass::CreateVTKObjects();
    this->EndCreateVTKObjects();
    if (this->ViewInformation)
      {
      // If ViewInformation is set, we now process it (after all objects have
      // been created to ensure that the subproxies etc. are setup which may be
      // affected by the values in the view information).
      this->ProcessViewInformation();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::BeginCreateVTKObjects()
{
  this->CacheKeeper = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("CacheKeeper"));
  this->CacheKeeper->SetServers(vtkProcessModule::DATA_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::UpdatePipeline()
{
  // Update the CacheKeeper.
  bool cachingEnabled = this->GetUseCache();
  vtkSMPropertyHelper(this->CacheKeeper, "CachingEnabled").Set(
    cachingEnabled? 1 : 0);
  vtkSMPropertyHelper(this->CacheKeeper, "CacheTime").Set(this->CacheTime);
  this->CacheKeeper->UpdateVTKObjects();
  if (cachingEnabled)
    {
    this->SomethingCached = true;
    }


  this->DataValid = true;
  this->InformationValid = false; 
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::UpdateLODPipeline()
{
  this->LODDataValid = true;
  this->LODInformationValid = false;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::Connect(vtkSMProxy* producer,
  vtkSMProxy* consumer, const char* propertyname/*="Input"*/,
  int outputport/*=0*/)
{
  if (!propertyname)
    {
    vtkErrorMacro("propertyname cannot be NULL.");
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on the consumer " << consumer->GetXMLName());
    return;
    }
  if (ip)
    {
    ip->RemoveAllProxies();
    ip->AddInputConnection(producer, outputport);
    }
  else
    {
    pp->RemoveAllProxies();
    pp->AddProxy(producer);
    }
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::SetUseCache(bool use_cache)
{
  if (this->UseCache != use_cache)
    {
    this->UseCache = use_cache;
    if (this->UseCache)
      {
      // This ensures that the cache will be save at the first opportunity.
      this->InvalidatePipeline();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::ProcessViewInformation()
{
  if (this->ViewInformation->Has(vtkSMRenderViewProxy::USE_LOD()))
    {
    this->SetUseLOD(
      this->ViewInformation->Get(vtkSMRenderViewProxy::USE_LOD())>0);
    }
  else
    {
    vtkErrorMacro("Missing Key: USE_LOD()");
    }

  if (this->ViewInformation->Has(vtkSMViewProxy::USE_CACHE()))
    {
    this->SetUseCache(
      this->ViewInformation->Get(vtkSMViewProxy::USE_CACHE())>0);
    }
  else
    {
    vtkErrorMacro("Missing Key: USE_CACHE()");
    }

  if (this->ViewInformation->Has(vtkSMViewProxy::CACHE_TIME()))
    {
    this->CacheTime = 
      this->ViewInformation->Get(vtkSMViewProxy::CACHE_TIME());
    }
  else
    {
    vtkErrorMacro("Missing Key: CACHE_TIME()");
    }

  if (this->ViewInformation->Has(vtkSMRenderViewProxy::LOD_RESOLUTION()))
    {
    this->SetLODResolution(
      this->ViewInformation->Get(vtkSMRenderViewProxy::LOD_RESOLUTION()));
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableLOD: " << this->EnableLOD << endl;
  os << indent << "EnableCaching: " << this->EnableCaching << endl;
  os << indent << "KeepLODPipelineUpdated: " 
    << this->KeepLODPipelineUpdated << endl;
  os << indent << "RepresentedDataInformation: " 
    << this->RepresentedDataInformation << endl;
}


