/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositePartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositePartDisplay.h"

#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositePartDisplay);
vtkCxxRevisionMacro(vtkPVCompositePartDisplay, "1.26.2.2");


//----------------------------------------------------------------------------
vtkPVCompositePartDisplay::vtkPVCompositePartDisplay()
{
  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;

  this->CollectID.ID = 0;
  this->LODCollectID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositePartDisplay::~vtkPVCompositePartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->CollectID.ID)
    {
    if ( pvApp )
      {
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      pm->DeleteStreamObject(this->CollectID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->CollectID.ID = 0;
    }
  if (this->LODCollectID.ID)
    {
    if ( pvApp )
      {
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      pm->DeleteStreamObject(this->LODCollectID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->LODCollectID.ID = 0;
    }
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVCompositePartDisplay::CreateCollectionFilter(vtkPVApplication* pvApp)
{ 
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerID id = pm->NewStreamObject("vtkMPIMoveData");
  // Create a temporary input.
  // This is needed to get the output of the vtkDataSetToDataSetFitler
  vtkClientServerID id2;
  id2 = pm->NewStreamObject("vtkPolyData");
  pm->GetStream() << vtkClientServerStream::Invoke << id 
                  << "SetInput" << id2 
                  <<  vtkClientServerStream::End;
  pm->DeleteStreamObject(id2);
  // Default is pass through because it executes fastest.  
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetMoveModeToPassThrough"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID()
    << vtkClientServerStream::End;
  // create, SetPassThrough, and set the mToN connection
  // object on all servers and client
  pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
  // always set client mode
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << id << "SetServerToClient"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT);
  // if running in client mode
  // then set the server to be servermode
  if(pvApp->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if(pvApp->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << id << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }
  
  return id;
}

//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->Superclass::CreateParallelTclObjects(pvApp);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  // Create the collection filters which allow small models to render locally.  
  // They also redistributed data for SGI pipes option.
  // ===== Primary branch:

  // Different filter for  pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    this->CollectID = pm->NewStreamObject("vtkAllToNRedistributePolyData");
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetNumberOfProcesses"
      << pvApp->GetNumberOfPipes()
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else if (pvApp->GetUseTiledDisplay() || pvApp->GetCaveConfigurationFileName())
    { 
    this->CollectID = pm->NewStreamObject("vtkMPIMoveData");
    // Create a temporary input.
    // This is needed to get the output of the vtkDataSetToDataSetFitler
    vtkClientServerID id;
    id = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke << this->CollectID 
                    << "SetInput" << id 
                    <<  vtkClientServerStream::End;
    pm->DeleteStreamObject(id);
    // Default is pass through because it executes fastest.    
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

    // For the render server feature.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
    }
  else
    { 
    this->CollectID = this->CreateCollectionFilter(pvApp);
    }

  vtkClientServerStream cmd;
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CollectID << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CollectID << "AddObserver" << "EndEvent" << cmd
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // ===== LOD branch:
  // Different filter for pipe redistribution.
  if (pvApp->GetUseRenderingGroup())
    {
    this->LODCollectID = pm->NewStreamObject("vtkAllToNRedistributePolyData");
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetNumberOfProcesses"
      << pvApp->GetNumberOfPipes()
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else if (pvApp->GetUseTiledDisplay() || pvApp->GetCaveConfigurationFileName())
    { // This should be in subclass.
    this->LODCollectID = pm->NewStreamObject("vtkMPIMoveData");
    // Create a temporary input.
    // This is needed to get the output of the vtkDataSetToDataSetFitler
    vtkClientServerID id;
    id = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke << this->CollectID 
                    << "SetInput" << id 
                    <<  vtkClientServerStream::End;
    pm->DeleteStreamObject(id);
    // Default is pass through because it executes fastest.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

    // For the render server feature.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
    }
  else
    {
    this->LODCollectID = this->CreateCollectionFilter(pvApp);
    }

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "SetInput"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "AddObserver" << "EndEvent" << cmd
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->CollectID << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Special condition to signal the client.
  // Because both processes of the Socket controller think they are 0!!!!
  if (pvApp->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetController" << 0
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetController" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }

  // Insert collection filters into pipeline.
  if (this->CollectID.ID)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }

  if (this->LODCollectID.ID)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "GetOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    }

  // We need to connect the geometry filter 
  // now that it is in the part display.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->GeometryID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODDeciID << "SetInput"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->GeometryID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->CollectID << "SetInput"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
}



//-----------------------------------------------------------------------------
// Updates if necessary.
vtkPVLODPartDisplayInformation* vtkPVCompositePartDisplay::GetInformation()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    vtkErrorMacro("Missing application.");
    return NULL;
    }
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  if ( ! this->GeometryIsValid)
    { // Update but with collection filter off.
    this->CollectionDecision = 0;
    this->LODCollectionDecision = 0;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    this->SendForceUpdate();
    this->InformationIsValid = 0;
    }

  return this->Superclass::GetInformation();
}


//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::SetCollectionDecision(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (v == this->CollectionDecision)
    {
    return;
    }
  this->CollectionDecision = v;
  if ( !this->UpdateSuppressorID.ID )
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }
  if(!pvApp)
    {
    return;
    }
  
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  if (this->CollectID.ID)
    {
    if (this->CollectionDecision)
      {
      if (pvApp->GetUseTiledDisplay())
        {
        pm->GetStream()
          << vtkClientServerStream::Invoke
          << this->CollectID << "SetMoveModeToClone"
          << vtkClientServerStream::End;
        }
      else
        {
        pm->GetStream()
          << vtkClientServerStream::Invoke
          << this->CollectID << "SetMoveModeToCollect"
          << vtkClientServerStream::End;
        }
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CollectID << "SetMoveModeToPassThrough"
        << vtkClientServerStream::End;
      }
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    }

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "RemoveAllCaches"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::SetLODCollectionDecision(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (v == this->LODCollectionDecision)
    {
    return;
    }
  this->LODCollectionDecision = v;

  if ( !this->LODUpdateSuppressorID.ID )
    {
    vtkErrorMacro("Missing Suppressor.");
    return;
    }
  if(!pvApp)
    {
    return;
    }
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  if (this->LODCollectID.ID)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetPassThrough"
      << (this->LODCollectionDecision? 0:1)
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "RemoveAllCaches"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorID << "ForceUpdate"
      << vtkClientServerStream::End;
    }

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorID << "RemoveAllCaches"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
void vtkPVCompositePartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CollectID: " << this->CollectID << endl;
  os << indent << "LODCollectID: " << this->LODCollectID << endl;

  os << indent << "CollectionDecision: " 
     <<  this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " 
     <<  this->LODCollectionDecision << endl;

}
