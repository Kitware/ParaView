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
vtkCxxRevisionMacro(vtkSMClientDeliveryStrategyProxy, "1.11");
//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::vtkSMClientDeliveryStrategyProxy()
{
  this->ReductionProxy = 0;
  this->CollectProxy = 0;
  this->SetEnableLOD(false);
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::~vtkSMClientDeliveryStrategyProxy()
{
  this->CollectProxy = 0;
  this->ReductionProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::SetPostGatherHelper(
  const char* classname)
{
  if (this->ReductionProxy)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->ReductionProxy->GetProperty("PostGatherHelper"));
    pp->RemoveAllProxies();
    pp = vtkSMProxyProperty::SafeDownCast(
      this->ReductionProxy->GetProperty("PreGatherHelper"));
    pp->RemoveAllProxies();
    this->ReductionProxy->UpdateVTKObjects();

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerID rfid;
    vtkClientServerStream stream;
    if (classname && classname[0])
      {
      rfid = pm->NewStreamObject(classname, stream);
      }

    stream  << vtkClientServerStream::Invoke
            << this->ReductionProxy->GetID() 
            << "SetPostGatherHelper"
            << rfid
            << vtkClientServerStream::End;

    if (!rfid.IsNull())
      {
      pm->DeleteStreamObject(rfid, stream);
      }

    pm->SendStream(this->ConnectionID, this->ReductionProxy->GetServers(),
      stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::SetPostGatherHelper(vtkSMProxy* helper)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ReductionProxy->GetProperty("PostGatherHelper"));
  pp->RemoveAllProxies();
  pp->AddProxy(helper);
  this->ReductionProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::SetPreGatherHelper(vtkSMProxy* helper)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ReductionProxy->GetProperty("PreGatherHelper"));
  pp->RemoveAllProxies();
  pp->AddProxy(helper);
  this->ReductionProxy->UpdateVTKObjects();
}
//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->ReductionProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Reduction"));

  this->CollectProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);
  this->ReductionProxy->SetServers(this->Servers);

  this->UpdateSuppressor->SetServers(this->Servers|vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(input, this->ReductionProxy, "Input", outputport);
  this->Connect(this->ReductionProxy, this->CollectProxy);
  this->Connect(this->CollectProxy, this->UpdateSuppressor);

  // Now we need to set up some default parameters on these filters.
  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() 
         << "SetProcessModuleConnection"
         << pm->GetConnectionClientServerID(this->GetConnectionID())
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->CollectProxy->GetServers(), stream);

  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->ReductionProxy->GetID() << "SetController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->ReductionProxy->GetServers(), stream);

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << vtkProcessModule::GetProcessModule()->GetProcessModuleID() 
    << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressor->GetID() 
    << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << vtkProcessModule::GetProcessModule()->GetProcessModuleID() 
    << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressor->GetID() 
    << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    this->UpdateSuppressor->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipeline()
{
  this->UpdatePipelineInternal(this->CollectProxy, 
                               this->UpdateSuppressor);
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipelineInternal(
  vtkSMSourceProxy* collect, vtkSMSourceProxy* )
{
  vtkSMSourceProxy* input = this->Input;
  if (input)
    {
    input->UpdatePipeline();
    vtkPVDataInformation* inputInfo = input->GetDataInformation(this->OutputPort);
    int dataType = inputInfo->GetDataSetType();
    int cDataType = inputInfo->GetCompositeDataSetType();
    if (cDataType > 0)
      {
      dataType = cDataType;
      }
    
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
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


