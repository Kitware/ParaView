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
void vtkSMSimpleStrategy::GatherInformation(vtkPVInformation* info)
{
  // It's essential that we don't call UpdatePipeline() on our subclasses since
  // they may still be in a state that's not fully determined eg. status of
  // compositing etc.
  this->vtkSMSimpleStrategy::UpdatePipeline();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    this->UpdateSuppressor->GetServers(),
    info,
    this->UpdateSuppressor->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherLODInformation(vtkPVInformation* info)
{
  // It's essential that we don't call UpdatePipeline() on our subclasses since
  // they may still be in a state that's not fully determined eg. status of
  // compositing etc.
  this->vtkSMSimpleStrategy::UpdateLODPipeline();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    this->UpdateSuppressorLOD->GetServers(),
    info,
    this->UpdateSuppressorLOD->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePipeline()
{
  // We check to see if the part of the pipeline that will up updated by this
  // class needs any update. Then alone do we call update.
  if (this->vtkSMSimpleStrategy::GetDataValid())
    {
    return;
    }

  // It's essential to call UpdatePipeline() on the superclass first since
  // that's the one that sets up CacheKeeper state correctly.
  this->Superclass::UpdatePipeline();
  
  this->UpdateSuppressor->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->UpdateSuppressor->UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdateLODPipeline()
{
  if (this->vtkSMSimpleStrategy::GetLODDataValid())
    {
    return;
    }

  this->Superclass::UpdateLODPipeline();

  this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
  // This is called for its side-effects; i.e. to force a PostUpdateData()
  this->UpdateSuppressorLOD->UpdatePipeline();
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


