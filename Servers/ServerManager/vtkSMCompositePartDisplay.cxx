/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositePartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositePartDisplay.h"

#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkClientServerStream.h"
#include "vtkSMProxy.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCompositePartDisplay);
vtkCxxRevisionMacro(vtkSMCompositePartDisplay, "1.4");


//----------------------------------------------------------------------------
vtkSMCompositePartDisplay::vtkSMCompositePartDisplay()
{
  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;

  this->CollectProxy = vtkSMProxy::New();
  this->CollectProxy->SetVTKClassName("vtkMPIMoveData");
  this->CollectProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);

  this->LODCollectProxy = vtkSMProxy::New();
  this->LODCollectProxy->SetVTKClassName("vtkMPIMoveData");
  this->LODCollectProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMCompositePartDisplay::~vtkSMCompositePartDisplay()
{
  this->CollectProxy->Delete();
  this->CollectProxy = 0;

  this->LODCollectProxy->Delete();
  this->LODCollectProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMCompositePartDisplay::SetupCollectionFilter(vtkSMProxy* collectProxy)
{ 
  vtkPVProcessModule* pm = static_cast<vtkPVProcessModule*>(
    vtkProcessModule::GetProcessModule());

  int i, num;
  
  vtkClientServerStream stream;

  num = collectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Default is pass through because it executes fastest.  
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
    // always set client mode
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT, stream);
    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      }
    }
}


//----------------------------------------------------------------------------
void vtkSMCompositePartDisplay::CreateVTKObjects(int num)
{
  vtkPVProcessModule* pm = static_cast<vtkPVProcessModule*>(
    vtkProcessModule::GetProcessModule());
  
  this->Superclass::CreateVTKObjects(num);

  int i;
  if (!this->CollectProxy->GetNumberOfIDs())
    {
    this->CollectProxy->UnRegisterVTKObjects();
    }
  if (!this->LODCollectProxy->GetNumberOfIDs())
    {
    this->LODCollectProxy->UnRegisterVTKObjects();
    }
  this->CollectProxy->CreateVTKObjects(num);
  this->LODCollectProxy->CreateVTKObjects(num);
  
  this->SetupCollectionFilter(this->CollectProxy);
  this->SetupCollectionFilter(this->LODCollectProxy);

  //law int fixme; // Use "SetupCollectionFilter" method.
  
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    // Create the collection filters which allow small models to render locally.  
    // They also redistributed data for SGI pipes option.
    // ===== Primary branch:
    if (pm->GetOptions()->GetTileDimensions()[0] || 
        pm->GetOptions()->GetCaveConfigurationFileName())
      { 
      // Default is pass through because it executes fastest.    
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetMoveModeToPassThrough"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

      // For the render server feature.
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
        << pm->GetMPIMToNSocketConnectionID()
        << vtkClientServerStream::End;
      pm->SendStream(
        vtkProcessModule::DATA_SERVER|vtkProcessModule::RENDER_SERVER, stream);
      }

    vtkClientServerStream cmd;
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
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // ===== LOD branch:
    if (pm->GetOptions()->GetTileDimensions()[0] || 
        pm->GetOptions()->GetCaveConfigurationFileName())
      { // This should be in subclass.
      // Default is pass through because it executes fastest.
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetMoveModeToPassThrough"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

      // For the render server feature.
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
        << pm->GetMPIMToNSocketConnectionID()
        << vtkClientServerStream::End;
      pm->SendStream(
        vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
      }

    stream
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute LODCollect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute LODCollect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Handle collection setup with client server.
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Special condition to signal the client.
    // Because both processes of the Socket controller think they are 0!!!!
    if (pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }

    // Insert collection filters into pipeline.
    if (this->CollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
      }

    if (this->LODCollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
      }

    // We need to connect the geometry filter 
    // now that it is in the part display.
    stream
      << vtkClientServerStream::Invoke
      << this->GeometryProxy->GetID(i) << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODDeciProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->GeometryProxy->GetID(i) << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    }
}



//-----------------------------------------------------------------------------
// Updates if necessary.
vtkPVLODPartDisplayInformation* vtkSMCompositePartDisplay::GetLODInformation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm == NULL)
    {
    vtkErrorMacro("Missing process module.");
    return NULL;
    }

  if ( ! this->GeometryIsValid)
    { // Update but with collection filter off.
    this->CollectionDecision = 0;
    this->LODCollectionDecision = 0;
    int i, num;
    num = this->CollectProxy->GetNumberOfIDs();
    vtkClientServerStream stream;
    for (i = 0; i < num; ++i)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetMoveModeToPassThrough"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetMoveModeToPassThrough"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      }
    this->SendForceUpdate(&stream);
    this->LODInformationIsValid = 0;
    }

  return this->Superclass::GetLODInformation();
}


//----------------------------------------------------------------------------
void vtkSMCompositePartDisplay::SetCollectionDecision(int v)
{

  if (v == this->CollectionDecision)
    {
    return;
    }
  this->CollectionDecision = v;
  if ( !this->UpdateSuppressorProxy)
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if(!pm)
    {
    return;
    }
  vtkClientServerStream stream;

  int i, num;
  num = this->CollectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    if (this->CollectProxy)
      {
      if (this->CollectionDecision)
        {
        stream
          << vtkClientServerStream::Invoke
          << this->CollectProxy->GetID(i) << "SetMoveModeToCollect"
          << vtkClientServerStream::End;
        }
      else
        {
        stream
          << vtkClientServerStream::Invoke
          << this->CollectProxy->GetID(i) << "SetMoveModeToPassThrough"
          << vtkClientServerStream::End;
        }
      stream
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "RemoveAllCaches"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      }

    stream
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "RemoveAllCaches"
      << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
}


//----------------------------------------------------------------------------
void vtkSMCompositePartDisplay::SetLODCollectionDecision(int v)
{
  if (v == this->LODCollectionDecision)
    {
    return;
    }
  this->LODCollectionDecision = v;

  if ( !this->LODUpdateSuppressorProxy )
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if(!pm)
    {
    return;
    }

  vtkClientServerStream stream;
  int i, num;
  num = this->CollectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    if (this->LODCollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetPassThrough"
        << (this->LODCollectionDecision? 0:1)
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "RemoveAllCaches"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "ForceUpdate"
        << vtkClientServerStream::End;
      }

    stream
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "RemoveAllCaches"
      << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
}

//----------------------------------------------------------------------------
void vtkSMCompositePartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CollectProxy: " << this->CollectProxy << endl;
  os << indent << "LODCollectProxy: " << this->LODCollectProxy << endl;

  os << indent << "CollectionDecision: " 
     <<  this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " 
     <<  this->LODCollectionDecision << endl;

}
