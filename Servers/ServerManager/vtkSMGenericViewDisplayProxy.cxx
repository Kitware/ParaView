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
vtkCxxRevisionMacro(vtkSMGenericViewDisplayProxy, "1.2");

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
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->CollectProxy = this->GetSubProxy("Collect");
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

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

cout << __FILE__ << ":" << __LINE__ << " here" << endl;

  unsigned int i;
  vtkClientServerStream stream;
  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
    if (this->CollectProxy)
      {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }
    }
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
  if (stream.GetNumberOfMessages() > 0)
    {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }
    /*
  ip = vtkSMInputProperty::SafeDownCast(
  this->UpdateSuppressorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();
  */
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
  if ( vtkProcessModule::GetProcessModule()->IsRemote(this->GetConnectionID()) )
    {
    unsigned int i;

cout << __FILE__ << ":" << __LINE__ << " here" << endl;
    this->SetupCollectionFilter(this->CollectProxy);

cout << __FILE__ << ":" << __LINE__ << " here" << endl;
    for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
      {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
      vtkClientServerStream cmd;
      vtkClientServerStream stream;
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
        vtkProcessModule::CLIENT_AND_SERVERS, stream);

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
        vtkProcessModule::CLIENT_AND_SERVERS, stream);

      // Special condition to signal the client.
      // Because both processes of the Socket controller think they are 0!!!!
      if (pm->GetClientMode())
        {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
        stream
          << vtkClientServerStream::Invoke
          << this->CollectProxy->GetID(i) << "SetController" << 0
          << vtkClientServerStream::End;
        pm->SendStream(this->ConnectionID,
          vtkProcessModule::CLIENT, stream);
        }
      }

    //this->SetOrderedCompositing(0);
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetupCollectionFilter(vtkSMProxy* collectProxy)
{ 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int i, num;

  vtkClientServerStream stream;

  num = collectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
    // Default is pass through because it executes fastest.  
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToClone"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER,
      stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToClone"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT,
      stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
    // always set client mode
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT, stream);
    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::DATA_SERVER, stream);
      }
    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
cout << __FILE__ << ":" << __LINE__ << " here" << endl;
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::RENDER_SERVER, stream);
      }
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
  const char* method, int hasMultipleInputs)
{
  this->SetInput(input);
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkSMGenericViewDisplayProxy::GetOutput()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

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

