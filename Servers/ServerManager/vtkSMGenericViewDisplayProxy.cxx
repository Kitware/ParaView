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

#include "vtkObjectFactory.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkClientServerStream.h"
#include "vtkSMSourceProxy.h"
#include "vtkDataObject.h"
#include "vtkMPIMoveData.h"
#include "vtkDataSet.h"
#include "vtkPVOptions.h"

vtkStandardNewMacro(vtkSMGenericViewDisplayProxy);
vtkCxxRevisionMacro(vtkSMGenericViewDisplayProxy, "1.6");

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy::vtkSMGenericViewDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->CanCreateProxy = 0;
  this->Visibility = 1;

  this->Output = 0;
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
void vtkSMGenericViewDisplayProxy::CreateVTKObjects(int numObjects)
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

  this->Superclass::CreateVTKObjects(numObjects);
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

  this->CreateVTKObjects(num);

  vtkSMInputProperty* ip = 0;

  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(input);
  this->CollectProxy->UpdateVTKObjects();

  unsigned int i;
  vtkClientServerStream stream;
  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    if (this->CollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "GetOutputPort"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "SetInputConnection"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, 
      this->CollectProxy->GetServers(), 
      stream);
    }

  if ( vtkProcessModule::GetProcessModule()->IsRemote(this->GetConnectionID()))
    {
    this->SetupCollectionFilter(this->CollectProxy);

    for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
      {
      vtkClientServerStream cmd;
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

      cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
        << vtkClientServerStream::End;
      cmd.Reset();
      cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
                     this->CollectProxy->GetServers(),
                     stream);

      // Handle collection setup with client server.
      stream
        << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "GetSocketController"
        << pm->GetConnectionClientServerID(this->ConnectionID)
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetSocketController"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
                     this->CollectProxy->GetServers(),
                     stream);

      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetupCollectionFilter(
  vtkSMProxy* collectProxy)
{ 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int i, num;

  vtkClientServerStream stream;

  num = collectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToDataServer"
      << vtkClientServerStream::End;
    int mask = ~vtkProcessModule::CLIENT;
    pm->SendStream(this->ConnectionID,
                   collectProxy->GetServers() & mask,
                   stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::CLIENT,
                   stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::Update()
{
  vtkSMProperty *p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
  if (!p)
    {
    vtkErrorMacro("Failed to find property ForceUpdate on "
      "UpdateSuppressorProxy.");
    return;
    }
  p->Modified();
  this->UpdateVTKObjects();
  this->Superclass::Update();
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
    
  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
    pm->GetObjectFromID(this->CollectProxy->GetID(0)));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutput();
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->Visibility << endl;
}

