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
#include "vtkPVApplication.h"
#include "vtkMPIDuplicateUnstructuredGrid.h"
#include "vtkPVDataInformation.h"
#include "vtkPVPart.h"
#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModule.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPickDisplay);
vtkCxxRevisionMacro(vtkPVPickDisplay, "1.2");


//----------------------------------------------------------------------------
vtkPVPickDisplay::vtkPVPickDisplay()
{
  this->PVApplication = NULL;

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
  vtkPVApplication *pvApp = this->GetPVApplication();
  if(pvApp)
    {
    vtkPVProcessModule* pm;
    pm = pvApp->GetProcessModule();  
    if (pm && this->DuplicateID.ID)
      {
      pm->DeleteStreamObject(this->DuplicateID);
      pm->SendStreamToRenderServerClientAndServer();
      }
    this->DuplicateID.ID = 0;
    if (pm && this->UpdateSuppressorID.ID)
      {
      pm->DeleteStreamObject(this->UpdateSuppressorID);
      pm->SendStreamToRenderServerClientAndServer();
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
    pm->SendStreamToClientAndRenderServer();
    }

  this->SetPart(NULL);
  this->SetPVApplication( NULL);
}


//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkPVPickDisplay::GetCollectedData()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  if (pvApp == NULL)
    {
    return NULL;
    }
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  if (pm == NULL)
    {
    return NULL;
    }
  vtkMPIDuplicateUnstructuredGrid* dp;
  dp = vtkMPIDuplicateUnstructuredGrid::SafeDownCast(
      pm->GetObjectFromID(this->DuplicateID));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutput();
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();

  // Create the fliter wich duplicates the data on all processes.
  this->DuplicateID = pm->NewStreamObject("vtkMPIDuplicateUnstructuredGrid");
  pm->SendStreamToRenderServerClientAndServer();
  if(pvApp->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->DuplicateID << "SetClientFlag" << 1
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
    }
  // Handle collection setup with client server.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->DuplicateID << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  this->UpdateSuppressorID = pm->NewStreamObject("vtkPVUpdateSuppressor");
  pm->SendStreamToRenderServerClientAndServer();
  stream << vtkClientServerStream::Invoke << this->DuplicateID << "GetOutput" 
         <<  vtkClientServerStream::End
         << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "SetInput" << vtkClientServerStream::LastResult 
         << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();

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
  pm->SendStreamToClientAndRenderServer();

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
  pm->SendStreamToRenderServerClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetPointLabelVisibility(int val)
{
  if (!this->PointLabelMapperID.ID || !this->PointLabelActorID.ID)
    {
    return;
    }
  
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (!pvApp)
    {
    return;
    }
  if (pvApp->GetRenderModule() == NULL)
    { // I had a crash on exit because render module was NULL.
    return;
    }  


  vtkPVProcessModule *pm = pvApp->GetProcessModule();

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
      << pvApp->GetRenderModule()->GetRendererID() << "AddProp"
      << this->PointLabelActorID << vtkClientServerStream::End;
    }
  else
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pvApp->GetRenderModule()->GetRendererID() << "RemoveProp"
      << this->PointLabelActorID << vtkClientServerStream::End;
    }
  pm->SendStreamToClientAndRenderServer();
}



//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetInput(vtkPVPart* input)
{
  vtkPVProcessModule* pm;

  vtkPVApplication *pvApp = this->GetPVApplication();
  if( ! pvApp)
    {
    vtkErrorMacro("Missing Application.");
    return;
    }
  pm = pvApp->GetProcessModule();  

  // Set vtkData as input to duplicate filter.
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->DuplicateID 
                  << "SetInput" << input->GetVTKDataID() 
                  << vtkClientServerStream::End;
  // Only the server has data.
  pm->SendStreamToServer();
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
  vtkPVApplication* pvApp = this->GetPVApplication();
  // Current problem is that there is no input for the UpdateSuppressor object
  if ( ! this->GeometryIsValid && this->UpdateSuppressorID.ID != 0 )
    {
    vtkPVProcessModule *pm = pvApp->GetProcessModule();
    vtkClientServerStream& stream = pm->GetStream();
    stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
           << "ForceUpdate" << vtkClientServerStream::End;
    pm->SendStreamToRenderServerClientAndServer();
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::SetPVApplication(vtkPVApplication *pvApp)
{
  if (pvApp == NULL)
    {
    if (this->PVApplication)
      {
      this->PVApplication->Delete();
      this->PVApplication = NULL;
      }
    return;
    }

  if (this->PVApplication)
    {
    vtkErrorMacro("PVApplication already set and part has been initialized.");
    return;
    }

  this->CreateParallelTclObjects(pvApp);
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);
}

//----------------------------------------------------------------------------
void vtkPVPickDisplay::RemoveAllCaches()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "RemoveAllCaches" << vtkClientServerStream::End; 
  pm->SendStreamToRenderServerClientAndServer();
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPickDisplay::CacheUpdate(int idx, int total)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule *pm = pvApp->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->UpdateSuppressorID 
         << "CacheUpdate" << idx << total << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();
}


//----------------------------------------------------------------------------
void vtkPVPickDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "Part: " << this->Part << endl;
  os << indent << "PVApplication: " << this->PVApplication << endl;
  os << indent << "UpdateSuppressorID: " << this->UpdateSuppressorID.ID << endl;
}

