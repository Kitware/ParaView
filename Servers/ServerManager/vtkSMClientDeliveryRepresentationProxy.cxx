/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientDeliveryRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientDeliveryRepresentationProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMClientDeliveryRepresentationProxy);
vtkCxxRevisionMacro(vtkSMClientDeliveryRepresentationProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::vtkSMClientDeliveryRepresentationProxy()
{
  this->ReduceProxy = 0;
  this->StrategyProxy = 0;

  this->ReductionType = 0;
  this->ExtractSelection = 0;

  this->SetSelectionSupported(false);
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::~vtkSMClientDeliveryRepresentationProxy()
{
  this->StrategyProxy->Delete();
  this->StrategyProxy = 0;

  this->ExtractSelection = 0;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::SetupStrategy()
{
  this->StrategyProxy = vtkSMClientDeliveryStrategyProxy::New();
  if (!this->StrategyProxy)
    {
    vtkErrorMacro("Failed to create vtkSMClientDeliveryStrategyProxy.");
    return false;
    }

  this->AddStrategy(this->StrategyProxy);

  this->StrategyProxy->SetEnableLOD(false);

  // Creates the strategy objects.
  this->StrategyProxy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  if(this->ReduceProxy)
    {
    this->Connect(this->ReduceProxy, this->StrategyProxy);
    }
  else
    {
    this->Connect(this->GetInputProxy(), this->StrategyProxy);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Initialize selection pipeline subproxies.
  this->ExtractSelection = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ExtractSelection"));

  this->ExtractSelection->SetServers(vtkProcessModule::DATA_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::EndCreateVTKObjects()
{
  // Setup selection pipeline connections.
  this->Connect(this->GetInputProxy(), this->ExtractSelection);

  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetReductionType(int type)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot set reduction type before CreateVTKObjects().");
    return;
    }

  if(this->ReductionType == type)
    {
    return;
    }

  this->ReductionType = type;

  if (!this->ReduceProxy)
    {
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  const char* classname = 0;
  switch (type)
    {
  case ADD:
    classname = "vtkAttributeDataReductionFilter";
    break;

  case POLYDATA_APPEND:
    classname = "vtkAppendPolyData";
    break;

  case UNSTRUCTURED_APPEND:
    classname = "vtkAppendFilter";
    break;

  case FIRST_NODE_ONLY:
    classname = 0;
    break;

  case RECTILINEAR_GRID_APPEND:
    classname = "vtkAppendRectilinearGrid";
    break;

  default:
    vtkErrorMacro("Unknown reduction type: " << type);
    return;
    }

  vtkClientServerID rfid;
  if ( classname )
    {
    rfid = pm->NewStreamObject(classname, stream);
    }
  stream
    << vtkClientServerStream::Invoke
    << this->ReduceProxy->GetID() << "SetPostGatherHelper"
    << rfid
    << vtkClientServerStream::End;

  if ( classname )
    {
    pm->DeleteStreamObject(rfid, stream);
    }

  if (type == FIRST_NODE_ONLY && this->ReduceProxy)
    {
    // We re-arrange the pipeline to remove the ReduceProxy
    // from the pipeline.
    }

  pm->SendStream(this->GetConnectionID(),
    this->ReduceProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetInputInternal()
{
  vtkSMSourceProxy* input = this->GetInputProxy();
  if(!input)
    {
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMInputProperty* ip = 0;

  if (this->ReduceProxy)
    {
    this->Connect(input, this->ReduceProxy);

    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetController"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->ReduceProxy->GetID() << "SetController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, 
        this->ReduceProxy->GetServers(), stream);
    }

  this->SetupStrategy();
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkSMClientDeliveryRepresentationProxy::GetOutput()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm || !this->StrategyProxy)
    {
    return NULL;
    }

  vtkAlgorithm* dp;
  vtkSMSourceProxy* pOutput = this->StrategyProxy->GetOutput();
  if (pOutput)
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(pOutput->GetID())); 
    }
  else
    {
    dp = NULL;
    }

  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutputDataObject(0);
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::AddInput(vtkSMSourceProxy* input, 
  const char* method, int hasMultipleInputs)
{
  this->Superclass::AddInput(input, method, hasMultipleInputs);
  this->SetInputInternal();
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


