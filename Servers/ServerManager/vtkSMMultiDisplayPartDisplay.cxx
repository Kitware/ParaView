/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiDisplayPartDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiDisplayPartDisplay.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMultiDisplayPartDisplay);
vtkCxxRevisionMacro(vtkSMMultiDisplayPartDisplay, "1.2");


//----------------------------------------------------------------------------
vtkSMMultiDisplayPartDisplay::vtkSMMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
vtkSMMultiDisplayPartDisplay::~vtkSMMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
void vtkSMMultiDisplayPartDisplay::SetLODCollectionDecision(int)
{
  // Always colect LOD.
  this->Superclass::SetLODCollectionDecision(1);
}

//----------------------------------------------------------------------------
void vtkSMMultiDisplayPartDisplay::Update()
{
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  this->Superclass::SetLODCollectionDecision(1);
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkSMMultiDisplayPartDisplay::CreateVTKObjects(int num)
{
  this->Superclass::CreateVTKObjects(num);
  vtkPVProcessModule* pm;
  
  pm = vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());
  int i;
  
  vtkClientServerStream stream;
  for (i = 0; i < num; ++i)
    {
    // This little hack causes collect mode to be iditical to clone mode.
    // This allows the superclass to treat tiled display like normal compositing.
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID(i) << "DefineCollectAsCloneOn"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODCollectProxy->GetID(i) << "DefineCollectAsCloneOn"
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    if(pm->GetClientMode())
      {
      // We need this because the socket controller has no way of distinguishing
      // between processes.
      stream << vtkClientServerStream::Invoke
             << this->CollectProxy->GetID(i) << "SetServerToClient"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->LODCollectProxy->GetID(i) << "SetServerToClient"
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      stream << vtkClientServerStream::Invoke
             << this->CollectProxy->GetID(i) << "SetServerToDataServer"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->LODCollectProxy->GetID(i) << "SetServerToDataServer"
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    else
      {
      vtkErrorMacro("Cannot run tile display without client-server mode.");
      }

    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
      stream << vtkClientServerStream::Invoke
             << this->CollectProxy->GetID(i) << "SetServerToRenderServer"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->LODCollectProxy->GetID(i) << "SetServerToRenderServer"
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      }  
    }
}

//----------------------------------------------------------------------------
void vtkSMMultiDisplayPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



