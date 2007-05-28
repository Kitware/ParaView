/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGenericViewDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGenericViewDisplayProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMGenericViewDisplayProxy);
vtkCxxRevisionMacro(vtkSMGenericViewDisplayProxy, "1.23");

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy::vtkSMGenericViewDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->ReduceProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->CanCreateProxy = 0;
  this->Visibility = 1;

  this->Output = 0;
  this->UpdateRequiredFlag = 1;

  this->ReductionType = 0;
}

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy::~vtkSMGenericViewDisplayProxy()
{
  if ( this->Output )
    {
    this->Output->Delete();
    this->Output = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);
  if (modifiedProxy != this)
    {
    this->UpdateRequiredFlag= 1;
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->UpdateSuppressorProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);
    
  this->CollectProxy = this->GetSubProxy("Collect");
  this->CollectProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);

  this->ReduceProxy =  this->GetSubProxy("Reduce");
  if (this->ReduceProxy)
    {
    this->ReduceProxy->SetServers(this->Servers);
    }

  this->PostProcessorProxy = this->GetSubProxy("PostProcessor");
  if (this->PostProcessorProxy)
    {
    this->PostProcessorProxy->SetServers(vtkProcessModule::CLIENT);
    }

  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetReductionType(int type)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot set reduction type before CreateVTKObjects().");
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
void vtkSMGenericViewDisplayProxy::SetInput(vtkSMProxy* sinput)
{
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(sinput);
  int num = 0;
  if (input)
    {
    num = input->GetNumberOfParts();
    if (!num)
      {
      input->CreateParts();
      num = input->GetNumberOfParts();
      }
    }
  if (num == 0)
    {
    vtkErrorMacro("Input proxy has no output! Cannot create the display");
    return;
    }

  // This will create all the subproxies with correct number of parts.
  if (input)
    {
    this->CanCreateProxy = 1;
    }

  this->CreateVTKObjects();

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMInputProperty* ip = 0;

  if (this->ReduceProxy)
    {
    ip = vtkSMInputProperty::SafeDownCast(
      this->ReduceProxy->GetProperty("Input"));
    ip->RemoveAllProxies();
    ip->AddProxy(input);
    this->ReduceProxy->UpdateVTKObjects();

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

  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  if (this->ReduceProxy)
    {
    ip->AddProxy(this->ReduceProxy);
    }
  else
    {
    ip->AddProxy(input);
    }

  this->CollectProxy->UpdateVTKObjects();

  stream
    << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID() << "SetProcessModuleConnection"
    << pm->GetConnectionClientServerID(this->GetConnectionID())
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 this->CollectProxy->GetServers(), stream);

  ip = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();


  if ( vtkProcessModule::GetProcessModule()->IsRemote(this->GetConnectionID()))
    {
    vtkClientServerStream cmd;
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID() << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID() << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->CollectProxy->GetServers(),
                   stream);
    }

  if (this->PostProcessorProxy)
    {
    ip = vtkSMInputProperty::SafeDownCast(
      this->PostProcessorProxy->GetProperty("Input"));
    ip->RemoveAllProxies();
    ip->AddProxy(this->CollectProxy);
    this->PostProcessorProxy->UpdateVTKObjects();
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
    << this->UpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << vtkProcessModule::GetProcessModule()->GetProcessModuleID() 
    << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    this->UpdateSuppressorProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetUpdateTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created!");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);
  // UpdateTime is immediate update, so no need to update.

  // Go upstream to the reader and mark it modified.
  vtkSMProxy* current = this;
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    current->GetProperty("Input"));
  while (current && pp && pp->GetNumberOfProxies() > 0)
    {
    current = pp->GetProxy(0);
    pp = vtkSMProxyProperty::SafeDownCast(current->GetProperty("Input"));
    }

  if (current)
    {
    current->MarkModified(current);
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::Update(vtkSMAbstractViewModuleProxy* view)
{
  // Set the output type of the collect filter based on the input data
  // type.
  vtkSMSourceProxy* input = 0;
  vtkSMInputProperty* inProp = vtkSMInputProperty::SafeDownCast(
    this->GetProperty("Input"));
  if (inProp && inProp->GetNumberOfProxies() == 1)
    {
    input = vtkSMSourceProxy::SafeDownCast(inProp->GetProxy(0));
    }
  if (input)
    {
    input->UpdatePipeline();
    vtkPVDataInformation* inputInfo = input->GetDataInformation();
    int dataType = inputInfo->GetDataSetType();

    vtkClientServerStream stream;

    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID() << "SetOutputDataType" << dataType
           << vtkClientServerStream::End;

    if (dataType == VTK_STRUCTURED_POINTS ||
        dataType == VTK_STRUCTURED_GRID   ||
        dataType == VTK_RECTILINEAR_GRID  ||
        dataType == VTK_IMAGE_DATA)
      {
      const int* extent = inputInfo->GetExtent();
      stream << vtkClientServerStream::Invoke
             << this->CollectProxy->GetID() 
             << "SetWholeExtent" 
             << vtkClientServerStream::InsertArray(extent, 6)
             << vtkClientServerStream::End;
      }

    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      this->CollectProxy->GetServers(), 
      stream);
    }
      
  this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
  this->Superclass::Update(view);
  this->UpdateRequiredFlag = 0;

  if (this->PostProcessorProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID())); 
    if (!dp)
      {
      vtkErrorMacro("Failed to get algorithm for PostProcessorProxy.");
      }
    else
      {
      dp->Update();
      }
    }

  this->InvokeEvent(vtkSMAbstractDisplayProxy::ForceUpdateEvent);
}

//-----------------------------------------------------------------------------
int vtkSMGenericViewDisplayProxy::UpdateRequired()
{
  return this->UpdateRequiredFlag;
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::AddInput(vtkSMSourceProxy* input,
  const char* vtkNotUsed(method), int vtkNotUsed(hasMultipleInputs))
{
  this->SetInput(input);
}
//-----------------------------------------------------------------------------
vtkDataObject* vtkSMGenericViewDisplayProxy::GetOutput()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm || !this->CollectProxy)
    {
    return NULL;
    }

  vtkAlgorithm* dp;
  if (this->PostProcessorProxy)
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID())); 
    }
  else
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->CollectProxy->GetID()));
    }

  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutputDataObject(0);
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->Visibility << endl;
}

