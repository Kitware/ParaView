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
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMRepresentationStrategy, "1.5");
//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::vtkSMRepresentationStrategy()
{
  this->Input = 0;
  this->ViewHelperProxy = 0;
  this->EnableLOD = false;
 
  this->LODDataValid = false;
  this->LODInformation = vtkPVDataInformation::New();;
  this->LODInformationValid = false;

  this->DataValid = false;
  this->Information = vtkPVDataInformation::New();
  this->InformationValid = false;

  this->SomethingCached = false;

  vtkMemberFunctionCommand<vtkSMRepresentationStrategy>* observer =
    vtkMemberFunctionCommand<vtkSMRepresentationStrategy>::New();
  observer->SetCallback(*this, 
    &vtkSMRepresentationStrategy::LODResolutionChanged);
  this->LODResolutionObserver = observer;
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::~vtkSMRepresentationStrategy()
{
  this->SetInput(0);
  this->SetViewHelperProxy(0);

  this->LODInformation->Delete();
  this->Information->Delete();
  this->LODResolutionObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::SetViewHelperProxy(vtkSMProxy* viewHelper)
{
  vtkSMProperty* prop = 0;
  if (this->ViewHelperProxy && 
    (prop = this->ViewHelperProxy->GetProperty("LODResolution")))
    {
    prop->RemoveObserver(this->LODResolutionObserver);
    }

  vtkSetObjectBodyMacro(ViewHelperProxy, vtkSMProxy, viewHelper);

  if (this->ViewHelperProxy &&
    (prop = this->ViewHelperProxy->GetProperty("LODResolution")))
    {
    prop->AddObserver(vtkCommand::ModifiedEvent, 
      this->LODResolutionObserver);
    this->LODResolutionChanged();
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::LODResolutionChanged()
{
  vtkSMProperty* myProp = this->GetProperty("LODResolution");
  vtkSMProperty* viewProp = this->ViewHelperProxy->GetProperty("LODResolution");
  if (myProp && viewProp)
    {
    myProp->Copy(viewProp);
    this->UpdateProperty("LODResolution");

    // Invalidate LOD data.
    this->LODDataValid = false;
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
  this->DataValid = false;
  this->LODDataValid = false;

  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->SomethingCached && !this->UseCache())
    {
    this->SomethingCached = false;
    this->InvokeCommand("RemoveAllCaches");
    }
  
  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationStrategy::GetDisplayedDataInformation()
{
  if (this->UseLODPipeline())
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
    this->LODInformation->Delete();
    this->LODInformation = vtkPVDataInformation::New();
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
    this->Information->Delete();
    this->Information = vtkPVDataInformation::New();
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
bool vtkSMRepresentationStrategy::UseLODPipeline()
{
  return (this->EnableLOD && !this->UseCache() &&
    this->ViewHelperProxy && 
    vtkSMRepresentationStrategyGetInt(this->ViewHelperProxy,"LODFlag", 0));
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::UseCache()
{
  return (this->ViewHelperProxy && 
    vtkSMRepresentationStrategyGetInt(
      this->ViewHelperProxy, "CachingEnabled", 0));
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationStrategy::UpdateRequired()
{
  // Since fullres is pipeline always needs to be up-to-date, if it is not
  // up-to-date, we need an Update. Additionally, is LOD is enabled and LOD
  // pipeline is not up-to-date, then too, we need an update.

  bool update_required = !this->DataValid;

  if (this->UseLODPipeline())
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

    if (this->UseLODPipeline() && !this->LODDataValid)
      {
      this->UpdateLODPipeline();
      }

    this->InvokeEvent(vtkCommand::EndEvent);
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::SetInput(vtkSMSourceProxy* input)
{
  vtkSetObjectBodyMacro(Input, vtkSMSourceProxy, input);
  if (!this->Input)
    {
    return;
    }

  // Not using the input number of parts here since that logic
  // is going to disappear in near future.
  this->CreateVTKObjects(1);

  this->CreatePipeline(this->Input);

  // LOD pipeline is created irrespective of if EnableLOD is true.
  this->CreateLODPipeline(this->Input);
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
  vtkSMProxy* consumer, const char* propertyname/*="Input"*/)
{
  if (!propertyname)
    {
    vtkErrorMacro("propertyname cannot be NULL.");
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on the consumer " << consumer->GetXMLName());
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(producer);
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableLOD: " << this->EnableLOD << endl;
  os << indent << "ViewHelperProxy: " << this->ViewHelperProxy << endl;
}


