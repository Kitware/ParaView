/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiDisplayPartDisplay.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayPartDisplay);
vtkCxxRevisionMacro(vtkPVMultiDisplayPartDisplay, "1.14");


//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::~vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::SetLODCollectionDecision(int)
{
  // Always colect LOD.
  this->Superclass::SetLODCollectionDecision(1);
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::Update()
{
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  this->Superclass::SetLODCollectionDecision(1);
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  this->Superclass::CreateParallelTclObjects(pvApp);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();


  // This little hack causes collect mode to be iditical to clone mode.
  // This allows the superclass to treat tiled display like normal compositing.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CollectID << "DefineCollectAsCloneOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "DefineCollectAsCloneOn"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);


  // pvApp->ClientMode is only set when there is a server.

  // A rather complex mess to set the correct server variable 
  // on all of the remote duplication filters.
  if(pvApp->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  else
    {
    vtkErrorMacro("Cannot run tile display without client-server mode.");
    }

  // if running in render server mode
  if(pvApp->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



