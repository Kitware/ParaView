/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointLabelDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPointLabelDisplay.h"
#include "vtkObjectFactory.h"
#include "vtkMPIMoveData.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModule.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPointLabelDisplay);
vtkCxxRevisionMacro(vtkSMPointLabelDisplay, "1.1");


//----------------------------------------------------------------------------
vtkSMPointLabelDisplay::vtkSMPointLabelDisplay()
{
  this->ProcessModule = NULL;

  this->Visibility = 1;
  this->GeometryIsValid = 0;

  this->UpdateSuppressorProxy = vtkSMProxy::New();
  this->UpdateSuppressorProxy->SetVTKClassName("vtkPVUpdateSuppressor");
  this->UpdateSuppressorProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->DuplicateProxy = vtkSMProxy::New();
  this->DuplicateProxy->SetVTKClassName("vtkMPIMoveData");
  this->DuplicateProxy->SetServersSelf(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->PointLabelMapperProxy = vtkSMProxy::New();
  this->PointLabelMapperProxy->SetVTKClassName("vtkLabeledDataMapper");
  this->PointLabelMapperProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
  this->PointLabelActorProxy = vtkSMProxy::New();
  this->PointLabelActorProxy->SetVTKClassName("vtkActor2D");
  this->PointLabelActorProxy->SetServersSelf(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);  
}

//----------------------------------------------------------------------------
vtkSMPointLabelDisplay::~vtkSMPointLabelDisplay()
{
  // This will remove the actor from the renderer.
  this->SetProcessModule(0);

  this->UpdateSuppressorProxy->Delete();
  this->UpdateSuppressorProxy = 0;
  this->DuplicateProxy->Delete();
  this->DuplicateProxy = 0;
  this->PointLabelMapperProxy->Delete();
  this->PointLabelMapperProxy = 0;
  this->PointLabelActorProxy->Delete();
  this->PointLabelActorProxy = 0;

}


//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkSMPointLabelDisplay::GetCollectedData()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == 0 || this->DuplicateProxy->GetNumberOfIDs() <= 0)
    {
    return 0;
    }
  vtkMPIMoveData* dp;
  dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->DuplicateProxy->GetID(0)));
  if (dp == 0)
    {
    return 0;
    }

  return dp->GetUnstructuredGridOutput();
}


//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::CreateVTKObjects(int num)
{
  vtkPVProcessModule* pm;
  pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  vtkClientServerStream& stream = pm->GetStream();

  if (num != 1)
    {
    vtkErrorMacro("PickFilter has multiple inputs, but only one output.");
    }

  this->UpdateSuppressorProxy->CreateVTKObjects(1);
  this->DuplicateProxy->CreateVTKObjects(1);
  this->PointLabelMapperProxy->CreateVTKObjects(1);
  this->PointLabelActorProxy->CreateVTKObjects(1);

  // A rather complex mess to set the correct server variable 
  // on all of the remote duplication filters.
  if(pm->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }
  // pm->ClientMode is only set when there is a server.
  if(pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if(pm->GetOptions()->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateProxy->GetID(0) << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  

  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetClientDataServerSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->DuplicateProxy->GetID(0) << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID()
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);

  stream << vtkClientServerStream::Invoke << this->DuplicateProxy->GetID(0) 
         << "SetMoveModeToClone" << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->PointLabelActorProxy->GetID(0) << "SetMapper" 
    << this->PointLabelMapperProxy->GetID(0)
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke 
    << this->UpdateSuppressorProxy->GetID(0) << "GetUnstructuredGridOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke 
    << this->PointLabelMapperProxy->GetID(0)
    << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  pm->GetStream()
    << vtkClientServerStream::Invoke 
    << this->DuplicateProxy->GetID(0) << "GetUnstructuredGridOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke 
    << this->UpdateSuppressorProxy->GetID(0)
    << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Tell the update suppressor to produce the correct partition.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID(0) << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID(0) << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::SetInput(vtkSMSourceProxy* input)
{  
  vtkPVProcessModule* pm;
  pm = this->GetProcessModule();  
  
  if (this->DuplicateProxy->GetNumberOfIDs() == 0)
    {
    this->CreateVTKObjects(1);
    }
  input->AddConsumer(0, this);

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicateProxy->GetID(0) 
                  << "SetInput" << input->GetPart(0)->GetID(0) 
                  << vtkClientServerStream::End;
  // Only the server has data.
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::AddToRenderer(vtkClientServerID rendererID)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == 0 || pm->GetRenderModule() == 0)
    { // I had a crash on exit because render module was NULL.
    return;
    }  

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  int i, num;
  num = this->PointLabelActorProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke << this->PointLabelMapperProxy->GetID(i)
      << "GetLabelTextProperty" << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult << "SetFontSize" << 24
      << vtkClientServerStream::End;    
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << rendererID << "AddProp"
      << this->PointLabelActorProxy->GetID(i) << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::RemoveFromRenderer(vtkClientServerID rendererID)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == 0 || pm->GetRenderModule() == 0)
    { // I had a crash on exit because render module was NULL.
    return;
    }  

  // There will be only one, but this is more general and protects
  // against the user calling this method before "MakeVTKObjects".
  int i, num;
  num = this->PointLabelActorProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << rendererID << "RemoveProp"
      << this->PointLabelActorProxy->GetID(i) << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}


//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::SetVisibility(int v)
{
  if (v)
    {
    v = 1;
    }
  if (v == this->Visibility)
    {
    return;
    }    
  
  this->Visibility = v;
    
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->PointLabelActorProxy->GetNumberOfIDs() > 0)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->PointLabelActorProxy->GetID(0) 
      << "SetVisibility" << v << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  // ....
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorProxy != 0 )
    {
    vtkPVProcessModule *pm = this->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorProxy->GetID(0) 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::SetProcessModule(vtkPVProcessModule *pm)
{
  if (pm == 0)
    {
    if (this->ProcessModule)
      {
      this->ProcessModule->Delete();
      this->ProcessModule = NULL;
      }
    return;
    }

  if (this->ProcessModule)
    {
    vtkErrorMacro("ProcessModule already set and part has been initialized.");
    return;
    }

  this->ProcessModule = pm;
  this->ProcessModule->Register(this);
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::RemoveAllCaches()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0) 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkSMPointLabelDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke 
         << this->UpdateSuppressorProxy->GetID(0) 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkSMPointLabelDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "ProcessModule: " << this->ProcessModule << endl;
  os << indent << "UpdateSuppressorProxy: " << this->UpdateSuppressorProxy << endl;
}

