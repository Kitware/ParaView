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
vtkCxxRevisionMacro(vtkSMSimpleStrategy, "1.14");
//----------------------------------------------------------------------------
vtkSMSimpleStrategy::vtkSMSimpleStrategy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
  this->SetEnableLOD(true);
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
void vtkSMSimpleStrategy::GatherLODInformation(vtkPVInformation* info)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    this->LODDecimator->GetServers(),
    info,
    this->LODDecimator->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePipeline()
{
  this->UpdateSuppressor->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->UpdateSuppressor->UpdatePipeline();

  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdateLODPipeline()
{
  this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");

  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->UpdateSuppressorLOD->UpdatePipeline();

  this->Superclass::UpdateLODPipeline();
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


