/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPickDisplay.h"
#include "vtkObjectFactory.h"
#include "vtkMPIMoveData.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModule.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPickDisplay);
vtkCxxRevisionMacro(vtkPVPickDisplay, "1.1");


//----------------------------------------------------------------------------
vtkPVPickDisplay::vtkPVPickDisplay()
{
  this->ProcessModule = NULL;

  this->Visibility = 1;
  this->Part = NULL;
  this->GeometryIsValid = 0;

  this->UpdateSuppressorID.ID = 0;
  this->DuplicateID.ID = 0;

  this->PointLabelMapperID.ID = 0;
  this->PointLabelActorID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVPickDisplay::~vtkPVPickDisplay()
{
  vtkPVProcessModule* pm;
  pm = this->GetProcessModule();  
  if (pm)
    {
    if (this->DuplicateID.ID)
      {
      pm->DeleteStreamObject(this->DuplicateID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->DuplicateID.ID = 0;
    if (this->UpdateSuppressorID.ID)
      {
      pm->DeleteStreamObject(this->UpdateSuppressorID);
      pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
      }
    this->UpdateSuppressorID.ID = 0;
    if (this->PointLabelMapperID.ID)
      {
      pm->DeleteStreamObject(this->PointLabelMapperID);
      this->PointLabelMapperID.ID = 0;
      }
    if (this->PointLabelActorID.ID)
      {
      pm->DeleteStreamObject(this->PointLabelActorID);
      this->PointLabelActorID.ID = 0;
      }
    // Is this necessary?
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
    
  this->SetPart(0);
  this->SetProcessModule(0);
}


//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPVPickDisplay::GetCollectedData()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm == NULL)
    {
    return NULL;
    }
  vtkMPIMoveData* dp;
  dp = vtkMPIMoveData::SafeDownCast(
      pm->GetObjectFromID(this->DuplicateID));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetUnstructuredGridOutput();
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  vtkClientServerStream& stream = pm->GetStream();

  // Create the fliter wich duplicates the data on all processes.
  this->DuplicateID = pm->NewStreamObject("vtkMPIMoveData");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // A rather complex mess to set the correct server variable 
  // on all of the remote duplication filters.
  if(pm->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateID << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }
  // pm->ClientMode is only set when there is a server.
  if(pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateID << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  // if running in render server mode
  if(pm->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateID << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  


  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->DuplicateID << "SetClientDataServerSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);

  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->DuplicateID << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID()
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);

  stream << vtkClientServerStream::Invoke << this->DuplicateID 
         << "SetMoveModeToClone" << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);


  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Set the output of the duplicate on all nodes.
  // Only the data server has an input and
  // the duplicate fitler is a vtkDataSetToDataSetFilter.
  vtkClientServerID id;
  id = pm->NewStreamObject("vtkUnstructuredGrid");
  pm->GetStream() << vtkClientServerStream::Invoke << this->DuplicateID 
                  << "SetInput" << id 
                  <<  vtkClientServerStream::End;
  // Now connect the duplicate filter to the update suppressor.
  pm->GetStream() << vtkClientServerStream::Invoke << this->DuplicateID
                  << "GetOutput" << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
                  << "SetInput" << vtkClientServerStream::LastResult 
                  << vtkClientServerStream::End;
  pm->DeleteStreamObject(id);
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  // Create point labels.
  this->PointLabelMapperID = pm->NewStreamObject("vtkLabeledDataMapper");
  this->PointLabelActorID = pm->NewStreamObject("vtkActor2D");
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->PointLabelActorID << "SetMapper" << this->PointLabelMapperID
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke << this->UpdateSuppressorID << "GetOutput"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke << this->PointLabelMapperID
    << "SetInput" << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  // Tell the update suppressor to produce the correct partition.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorID << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetPointLabelVisibility(int val)
{
  if (!this->PointLabelMapperID.ID || !this->PointLabelActorID.ID)
    {
    return;
    }
  
  vtkPVProcessModule *pm = this->GetProcessModule();
  if (pm->GetRenderModule() == NULL)
    { // I had a crash on exit because render module was NULL.
    return;
    }  


  pm->GetStream()
    << vtkClientServerStream::Invoke << this->PointLabelMapperID
    << "GetLabelTextProperty" << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << vtkClientServerStream::LastResult << "SetFontSize" << 24
    << vtkClientServerStream::End;
    
  if (val)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetRenderModule()->GetRendererID() << "AddProp"
      << this->PointLabelActorID << vtkClientServerStream::End;
    }
  else
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetRenderModule()->GetRendererID() << "RemoveProp"
      << this->PointLabelActorID << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}



//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetInput(vtkSMPart* input)
{
  vtkPVProcessModule* pm;

  pm = this->GetProcessModule();  

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicateID 
                  << "SetInput" << input->GetID(0) 
                  << vtkClientServerStream::End;
  // Only the server has data.
  pm->SendStream(vtkProcessModule::DATA_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetVisibility(int v)
{
  this->Visibility = v;
  // ....
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  // ....
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::Update()
{
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = this->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetProcessModule(vtkPVProcessModule *pm)
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

  this->CreateParallelTclObjects(pm);
  this->ProcessModule = pm;
  this->ProcessModule->Register(this);
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::RemoveAllCaches()
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPickDisplay::CacheUpdate(int idx, int total)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "Part: " << this->Part << endl;
  os << indent << "ProcessModule: " << this->ProcessModule << endl;
  os << indent << "UpdateSuppressorID: " << this->UpdateSuppressorID.ID << endl;
}

