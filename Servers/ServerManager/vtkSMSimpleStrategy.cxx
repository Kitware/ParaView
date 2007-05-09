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

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleStrategy);
vtkCxxRevisionMacro(vtkSMSimpleStrategy, "1.3");
//----------------------------------------------------------------------------
vtkSMSimpleStrategy::vtkSMSimpleStrategy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
}

//----------------------------------------------------------------------------
vtkSMSimpleStrategy::~vtkSMSimpleStrategy()
{

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->LODDecimator = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("LODDecimator"));
  this->UpdateSuppressor = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressorLOD"));

  this->LODDecimator->SetServers(vtkProcessModule::DATA_SERVER);
  this->UpdateSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->UpdateSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreateLODPipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->LODDecimator);
  this->Connect(this->LODDecimator, this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherInformation(vtkPVDataInformation* info)
{
  info->AddInformation(
    this->UpdateSuppressor->GetDataInformation());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherLODInformation(vtkPVDataInformation* info)
{
  info->AddInformation(
    this->UpdateSuppressorLOD->GetDataInformation());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePipeline()
{
  if (this->UseCache())
    {
    this->UpdateSuppressorLOD->InvokeCommand("CacheUpdate");
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
  // LODPipeline is never used when caching, hence we don't
  // need to check if caching is enabled.
  this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


