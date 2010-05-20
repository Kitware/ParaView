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
//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::vtkSMClientDeliveryStrategyProxy()
{
  this->ReductionProxy = 0;
  this->CollectProxy = 0;
  this->PostCollectUpdateSuppressor = 0;
  this->CollectedDataValid = false;
  this->SetEnableLOD(false);
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryStrategyProxy::~vtkSMClientDeliveryStrategyProxy()
{
  this->CollectProxy = 0;
  this->ReductionProxy = 0;
  this->PostCollectUpdateSuppressor = 0;
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
    this->CollectedDataValid = false;
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
  this->CollectedDataValid = false;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::SetPreGatherHelper(vtkSMProxy* helper)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ReductionProxy->GetProperty("PreGatherHelper"));
  pp->RemoveAllProxies();
  pp->AddProxy(helper);
  this->ReductionProxy->UpdateVTKObjects();
  this->CollectedDataValid = false;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();
  this->UpdateSuppressor->SetServers(this->Servers);

  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->ReductionProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Reduction"));

  this->CollectProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);
  this->ReductionProxy->SetServers(this->Servers);

  this->PostCollectUpdateSuppressor = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("PostCollectUpdateSuppressor"));
  this->PostCollectUpdateSuppressor->SetServers(
    this->Servers|vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::CreatePipeline(vtkSMSourceProxy* input,
  int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  this->Connect(this->Superclass::GetOutput(), this->ReductionProxy);
  this->Connect(this->ReductionProxy, this->CollectProxy);
  this->Connect(this->CollectProxy, this->PostCollectUpdateSuppressor);

  // Now set the MPI controller.
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID() << "GetController"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->ReductionProxy->GetID() << "SetController"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->ReductionProxy->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipeline()
{
  if (this->vtkSMClientDeliveryStrategyProxy::GetDataValid())
    {
    return;
    }

  this->Superclass::UpdatePipeline();
  this->UpdatePipelineInternal(this->CollectProxy, 
                               this->PostCollectUpdateSuppressor);

  this->PostCollectUpdateSuppressor->InvokeCommand("ForceUpdate");
  this->PostCollectUpdateSuppressor->UpdatePipeline();
  this->CollectedDataValid = true;
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::UpdatePipelineInternal(
  vtkSMSourceProxy* collect, vtkSMSourceProxy* )
{
  vtkPVDataInformation* inputInfo = this->GetRepresentedDataInformation();
  this->ReductionProxy->UpdatePipeline();
  vtkPVDataInformation* outputInfo = this->ReductionProxy->GetDataInformation(0);
  int dataType = outputInfo->GetDataSetType();
  int cDataType = outputInfo->GetCompositeDataSetType();
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

//----------------------------------------------------------------------------
void vtkSMClientDeliveryStrategyProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


