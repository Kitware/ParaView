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
#include "vtkPVGeometryInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMRepresentationStrategy, "1.14");
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
  this->LODInformation = vtkPVGeometryInformation::New();;
  this->LODInformationValid = false;
  this->LODResolution = 50;

  this->DataValid = false;
  this->Information = vtkPVGeometryInformation::New();
  this->InformationValid = false;

  vtkMemberFunctionCommand<vtkSMRepresentationStrategy>* command =
    vtkMemberFunctionCommand<vtkSMRepresentationStrategy>::New();
  command->SetCallback(*this,
    &vtkSMRepresentationStrategy::ProcessViewInformation);
  this->Observer = command;

  this->SomethingCached = false;
  this->KeepLODPipelineUpdated = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::~vtkSMRepresentationStrategy()
{
  this->AddInput(0, 0, 0, 0);
  this->SetViewInformation(0);

  this->LODInformation->Delete();
  this->Information->Delete();

  this->Observer->Delete();
  this->Observer = 0;
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
    // Get the current values from the view helper.
    this->ProcessViewInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::MarkModified(vtkSMProxy* modifiedProxy)
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
  
  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::InvalidatePipeline()
{
  this->DataValid = false;

  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->SomethingCached && !this->GetUseCache())
    {
    this->SomethingCached = false;
    this->InvokeCommand("RemoveAllCaches");
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::InvalidateLODPipeline()
{
  this->LODDataValid = false;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationStrategy::GetDisplayedDataInformation()
{
  if (this->GetUseLOD())
    {
    return this->GetLODDataInformation();
    }

  return this->GetFullResDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationStrategy::GetLODDataInformation()
{
  if (!this->LODInformationValid)
    {
    this->LODInformationValid = true;
    this->LODInformation->Initialize();
    this->GatherLODInformation(this->LODInformation);
    }
  return this->LODInformation;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationStrategy::GetFullResDataInformation()
{
  if (!this->InformationValid)
    {
    this->InformationValid = true;
    this->Information->Initialize();
    this->GatherInformation(this->Information);
    }

  return this->Information;
}

//----------------------------------------------------------------------------
inline int vtkSMRepresentationStrategyGetInt(vtkSMProxy* proxy, 
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

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::GetUseLOD()
{
  return (this->EnableLOD && !this->GetUseCache() && this->UseLOD);
;
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

  bool update_required = !this->DataValid;

  if (this->GetUseLOD() || (this->EnableLOD && this->KeepLODPipelineUpdated))
    {
    update_required |= !this->LODDataValid;
    }

  return update_required;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::Update()
{
  if (this->UpdateRequired())
    {
    this->InvokeEvent(vtkCommand::StartEvent);

    if (!this->DataValid)
      {
      this->UpdatePipeline();
      }

    if ( (this->GetUseLOD() || (this->EnableLOD && this->KeepLODPipelineUpdated)) 
      && !this->LODDataValid)
      {
      this->UpdateLODPipeline();
      }

    this->InvokeEvent(vtkCommand::EndEvent);
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

  this->CreatePipeline(this->Input, this->OutputPort);

  // LOD pipeline is created only if EnableLOD is true.
  if (this->EnableLOD)
    {
    this->CreateLODPipeline(this->Input, this->OutputPort);
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
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::UpdatePipeline()
{
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
}


