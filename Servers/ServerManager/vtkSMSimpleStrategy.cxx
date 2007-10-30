/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleStrategy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataSizeInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleStrategy);
vtkCxxRevisionMacro(vtkSMSimpleStrategy, "1.12");
//----------------------------------------------------------------------------
vtkSMSimpleStrategy::vtkSMSimpleStrategy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
  this->SetEnableLOD(true);
  this->SomethingCached = false;
  this->SomethingCachedLOD = false;
}

//----------------------------------------------------------------------------
vtkSMSimpleStrategy::~vtkSMSimpleStrategy()
{

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->LODDecimator = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("LODDecimator"));
  this->UpdateSuppressor = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressorLOD"));

  this->UpdateSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  if (this->LODDecimator && this->UpdateSuppressorLOD)
    {
    this->LODDecimator->SetServers(vtkProcessModule::DATA_SERVER);
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else
    {
    this->SetEnableLOD(false);
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->UpdateSuppressor, "Input", outputport);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->LODDecimator, "Input", outputport);
  this->Connect(this->LODDecimator, this->UpdateSuppressorLOD, "Input", 0);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherInformation(vtkPVInformation* info)
{
  this->UpdatePipeline();

  // For simple strategy information sub-pipline is same as the full pipeline
  // since no data movements are involved.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::DATA_SERVER_ROOT,
    info,
    this->UpdateSuppressor->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherLODInformation(vtkPVInformation* info)
{
  this->UpdateLODPipeline();

  // For simple strategy information sub-pipline is same as the full pipeline
  // since no data movements are involved.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::DATA_SERVER_ROOT,
    info,
    this->UpdateSuppressorLOD->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePipeline()
{
  if (this->GetUseCache())
    {
    this->SomethingCached = true;
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->UpdateSuppressor->GetProperty("CacheUpdate"));
    dvp->SetElement(0, this->CacheTime);
    this->UpdateSuppressor->UpdateProperty("CacheUpdate", 1);
    }
  else
    {
    this->UpdateSuppressor->InvokeCommand("ForceUpdate");
    }
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdateLODPipeline()
{
  if (this->GetUseCache())
    {
    this->SomethingCachedLOD = true;
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->UpdateSuppressorLOD->GetProperty("CacheUpdate"));
    dvp->SetElement(0, this->CacheTime);
    this->UpdateSuppressorLOD->UpdateProperty("CacheUpdate", 1);
    }
  else
    {
    this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
    }

  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->SomethingCached && !this->GetUseCache())
    {
    this->SomethingCached = false;
    this->UpdateSuppressor->InvokeCommand("RemoveAllCaches");
    }

  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::InvalidateLODPipeline()
{
  if (this->SomethingCachedLOD && !this->GetUseCache())
    {
    this->SomethingCachedLOD = false;
    this->UpdateSuppressorLOD->InvokeCommand("RemoveAllCaches");
    }

  this->Superclass::InvalidateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::SetLODResolution(int resolution)
{
  this->Superclass::SetLODResolution(resolution);

  if (this->LODDecimator)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LODDecimator->GetProperty("NumberOfDivisions"));
    if (ivp)
      {
      ivp->SetElement(0, this->LODResolution);
      ivp->SetElement(1, this->LODResolution);
      ivp->SetElement(2, this->LODResolution);
      this->LODDecimator->UpdateVTKObjects();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


