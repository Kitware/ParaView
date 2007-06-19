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
#include "vtkPVDataInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleStrategy);
vtkCxxRevisionMacro(vtkSMSimpleStrategy, "1.6.4.2");
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
void vtkSMSimpleStrategy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // Update piece information on each of the update suppressors.
  this->UpdatePieceInformation(this->UpdateSuppressor);
  if (this->GetEnableLOD())
    {
    this->UpdatePieceInformation(this->UpdateSuppressorLOD);
    }

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePieceInformation(vtkSMSourceProxy* updateSuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updateSuppressor->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updateSuppressor->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, updateSuppressor->GetServers(), stream);
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
  if (this->GetUseCache())
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
void vtkSMSimpleStrategy::UpdateLODPipeline()
{
  // LODPipeline is never used when caching, hence we don't
  // need to check if caching is enabled.
  this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
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


