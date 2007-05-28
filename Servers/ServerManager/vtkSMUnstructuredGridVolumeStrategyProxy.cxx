/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridVolumeStrategyProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridVolumeStrategyProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSMUnstructuredGridVolumeStrategyProxy);
vtkCxxRevisionMacro(vtkSMUnstructuredGridVolumeStrategyProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeStrategyProxy::vtkSMUnstructuredGridVolumeStrategyProxy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeStrategyProxy::~vtkSMUnstructuredGridVolumeStrategyProxy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::CreateVTKObjects()
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

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::CreateLODPipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->LODDecimator);
  this->Connect(this->LODDecimator, this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::GatherInformation(vtkPVDataInformation* info)
{
  info->AddInformation(
    this->UpdateSuppressor->GetDataInformation());
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::GatherLODInformation(vtkPVDataInformation* info)
{
  info->AddInformation(
    this->UpdateSuppressorLOD->GetDataInformation());
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::UpdatePipeline()
{
  if (this->UseCache())
    {
    this->SomethingCached = true;
    this->UpdateSuppressorLOD->InvokeCommand("CacheUpdate");
    }
  else
    {
    this->UpdateSuppressor->InvokeCommand("ForceUpdate");
    }
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::UpdateLODPipeline()
{
  // LODPipeline is never used when caching, hence we don't
  // need to check if caching is enabled.
  this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeStrategyProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


