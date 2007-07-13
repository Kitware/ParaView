/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUniformGridParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"

vtkStandardNewMacro(vtkSMUniformGridParallelStrategy);
vtkCxxRevisionMacro(vtkSMUniformGridParallelStrategy, "1.5");
//----------------------------------------------------------------------------
vtkSMUniformGridParallelStrategy::vtkSMUniformGridParallelStrategy()
{
  this->Collect = 0;
  this->CollectLOD = 0;
  this->SetEnableLOD(true);
  this->SetKeepLODPipelineUpdated(true);
}

//----------------------------------------------------------------------------
vtkSMUniformGridParallelStrategy::~vtkSMUniformGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::BeginCreateVTKObjects()
{
  this->Collect = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Collect"));
  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->CollectLOD =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CollectLOD"));
  this->CollectLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->Superclass::BeginCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();
  this->InitializeCollectProxy(this->Collect);
  this->InitializeCollectProxy(this->CollectLOD);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("MoveMode"));
  ivp->SetElement(0, vtkMPIMoveData::PASS_THROUGH);
  this->Collect->UpdateVTKObjects();

  // Collect filter must be told the output data type since the data may not be
  // available on all processess.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_IMAGE_DATA);
  this->Collect->UpdateVTKObjects();

  // CollectLOD on the other hand is used when rendering on the client side. 
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectLOD->GetProperty("MoveMode"));
  ivp->SetElement(0, vtkMPIMoveData::COLLECT);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectLOD->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_POLY_DATA);
  this->CollectLOD->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::InitializeCollectProxy(
  vtkSMProxy* collectProxy)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // Collect filter needs the socket controller use to communicate between
  // data-server root and the client.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() 
          << "GetSocketController"
          << pm->GetConnectionClientServerID(this->ConnectionID)
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << collectProxy->GetID()
          << "SetSocketController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT_AND_SERVERS, stream);

  // Collect filter needs the MPIMToNSocketConnection to communicate between
  // render server and data server nodes.
  stream  << vtkClientServerStream::Invoke
          << collectProxy->GetID()
          << "SetMPIMToNSocketConnection"
          << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  // Set the server flag on the collect filter to correctly identify each
  // processes.
  stream  << vtkClientServerStream::Invoke
          << collectProxy->GetID()
          << "SetServerToRenderServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collectProxy->GetID()
          << "SetServerToDataServer"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::DATA_SERVER, stream);
  stream  << vtkClientServerStream::Invoke
          << collectProxy->GetID()
          << "SetServerToClient"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Connect(input, this->Collect, "Input", outputport);
  this->Superclass::CreatePipeline(this->Collect, 0);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, 
  int outputport)
{
  this->Connect(input, this->LODDecimator, "Input", outputport);
  this->Connect(this->LODDecimator, this->CollectLOD);
  this->Connect(this->CollectLOD, this->UpdateSuppressorLOD);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


