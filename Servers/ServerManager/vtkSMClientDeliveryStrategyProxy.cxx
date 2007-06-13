/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientDeliveryStrategyProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientDeliveryStrategyProxy.h"

#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMClientDeliveryStrategyProxy);
vtkCxxRevisionMacro(vtkSMClientDeliveryStrategyProxy, "1.5");
//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::vtkSMClientDeliveryStrategyProxy()
{
  this->CollectProxy = 0;
  this->CollectLODProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::~vtkSMClientDeliveryStrategyProxy()
{
  this->CollectProxy = 0;
  this->CollectLODProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->CollectProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);

  this->CollectLODProxy = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));
  this->CollectLODProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->CreatePipelineInternal(input,
                               this->CollectProxy, 
                               this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreateLODPipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->LODDecimator);
  this->CreatePipelineInternal(this->LODDecimator,
                               this->CollectLODProxy, 
                               this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreatePipelineInternal(
  vtkSMSourceProxy* input,
  vtkSMSourceProxy* collect,
  vtkSMSourceProxy* updatesuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(input, collect);

  // Now we need to set up some default parameters on these filters.

  stream
    << vtkClientServerStream::Invoke
    << collect->GetID() << "SetProcessModuleConnection"
    << pm->GetConnectionClientServerID(this->GetConnectionID())
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 collect->GetServers(), stream);

  this->Connect(collect, updatesuppressor);

  if ( vtkProcessModule::GetProcessModule()->IsRemote(this->GetConnectionID()))
    {
    vtkClientServerStream cmd;
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collect->GetID() << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collect->GetID() << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, collect->GetServers(),
                   stream);
    }

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << vtkProcessModule::GetProcessModule()->GetProcessModuleID() 
    << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updatesuppressor->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << vtkProcessModule::GetProcessModule()->GetProcessModuleID() 
    << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updatesuppressor->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    updatesuppressor->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipeline()
{
  this->UpdatePipelineInternal(this->CollectProxy, 
                               this->UpdateSuppressor);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdateLODPipeline()
{
  this->UpdatePipelineInternal(this->CollectLODProxy, 
                               this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipelineInternal(
  vtkSMSourceProxy* collect, vtkSMSourceProxy* updatesuppressor)
{
  vtkSMSourceProxy* input = this->Input;
  if (input)
    {
    input->UpdatePipeline();
    vtkPVDataInformation* inputInfo = input->GetDataInformation();
    int dataType = inputInfo->GetDataSetType();

    vtkClientServerStream stream;

    stream << vtkClientServerStream::Invoke
           << collect->GetID() << "SetOutputDataType" << dataType
           << vtkClientServerStream::End;

    if (dataType == VTK_STRUCTURED_POINTS ||
        dataType == VTK_STRUCTURED_GRID   ||
        dataType == VTK_RECTILINEAR_GRID  ||
        dataType == VTK_IMAGE_DATA)
      {
      const int* extent = inputInfo->GetExtent();
      stream << vtkClientServerStream::Invoke
             << collect->GetID() 
             << "SetWholeExtent" 
             << vtkClientServerStream::InsertArray(extent, 6)
             << vtkClientServerStream::End;
      }

    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      collect->GetServers(), 
      stream);
    }
      
  updatesuppressor->InvokeCommand("ForceUpdate");
  this->Superclass::Update();
}
//-----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMClientDeliveryStrategyProxy::GetOutput()
{
  return this->CollectProxy;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


