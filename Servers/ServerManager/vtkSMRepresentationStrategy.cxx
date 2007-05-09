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

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkCxxRevisionMacro(vtkSMRepresentationStrategy, "1.2");
vtkCxxSetObjectMacro(vtkSMRepresentationStrategy, ViewHelperProxy, vtkSMProxy);
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
void vtkSMRepresentationStrategy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this)
    {
    // Mark all data invalid.
    // Note that we are not marking the information invalid. The information
    // won't get invalidated until the pipeline is updated.
    this->DataValid = false;
    this->LODDataValid = false;
    }
  
  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationStrategy::GetDisplayedDataInformation()
{
  if (this->UseLODPipeline())
    {
    if (!this->LODInformationValid)
      {
      this->LODInformationValid = true;
      this->GatherLODInformation(this->LODInformation);
      }
    return this->LODInformation;
    }

  if (!this->InformationValid)
    {
    this->InformationValid = true;
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
  // If using LOD, then update is required if LOD data is invalid,
  // otherwise update is required if non-LOD data is invalid.
  if (this->UseLODPipeline())
    {
    return !this->LODDataValid;
    }

  return !this->DataValid;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationStrategy::Update()
{
  if (this->UpdateRequired())
    {
    if (this->UseLODPipeline())
      {
      this->UpdateLODPipeline();
      }
    else
      {
      this->UpdatePipeline();
      }
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


