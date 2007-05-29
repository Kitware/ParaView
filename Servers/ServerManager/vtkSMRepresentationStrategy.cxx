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
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMRepresentationStrategy, "1.8");
//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::vtkSMRepresentationStrategy()
{
  this->UseCache = false;
  this->UseLOD = false;
  this->Input = 0;
  this->ViewHelperProxy = 0;
  this->EnableLOD = false;
  this->EnableCaching = true;
 
  this->LODDataValid = false;
  this->LODInformation = vtkPVDataInformation::New();;
  this->LODInformationValid = false;
  this->LODResolution = 50;

  this->DataValid = false;
  this->Information = vtkPVDataInformation::New();
  this->InformationValid = false;


  this->SomethingCached = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy::~vtkSMRepresentationStrategy()
{
  this->SetInput(0);
  this->SetViewHelperProxy(0);

  this->LODInformation->Delete();
  this->Information->Delete();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::SetViewHelperProxy(vtkSMProxy* viewHelper)
{
  vtkSetObjectBodyMacro(ViewHelperProxy, vtkSMProxy, viewHelper);

  // Get the current values from the view helper.
  this->ViewHelperModified();
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy == this->ViewHelperProxy)
    {
    this->ViewHelperModified();
    return;
    }

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

  if (this->GetUseLOD())
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

    if (this->GetUseLOD() && !this->LODDataValid)
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
  this->CreateVTKObjects();

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
void vtkSMRepresentationStrategy::ViewHelperModified()
{
  if (!this->ViewHelperProxy)
    {
    return;
    }

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ViewHelperProxy->GetProperty("UseLOD"));
  if (ivp && ivp->GetNumberOfElements() == 1)
    {
    this->SetUseLOD(ivp->GetElement(0) == 1);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ViewHelperProxy->GetProperty("UseCache"));
  if (ivp && ivp->GetNumberOfElements() == 1)
    {
    this->SetUseCache(ivp->GetElement(0) == 1);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ViewHelperProxy->GetProperty("LODResolution"));
  if (ivp && ivp->GetNumberOfElements() == 1)
    {
    this->SetLODResolution(ivp->GetElement(0));
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EnableLOD: " << this->EnableLOD << endl;
  os << indent << "EnableCaching: " << this->EnableCaching << endl;
  os << indent << "ViewHelperProxy: " << this->ViewHelperProxy << endl;
}


