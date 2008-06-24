/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNetworkImageSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNetworkImageSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMNetworkImageSourceProxy);
vtkCxxRevisionMacro(vtkSMNetworkImageSourceProxy, "1.6");
//----------------------------------------------------------------------------
vtkSMNetworkImageSourceProxy::vtkSMNetworkImageSourceProxy()
{
  this->FileName = 0;
  this->SourceProcess = CLIENT;
  this->UpdateNeeded = false;
  this->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->ForceNoUpdates = false;
}

//----------------------------------------------------------------------------
vtkSMNetworkImageSourceProxy::~vtkSMNetworkImageSourceProxy()
{
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::SetFileName(const char* fname)
{
  if (this->FileName && fname && strcmp(this->FileName, fname) == 0)
    {
    return;
    }
  delete [] this->FileName;
  this->FileName = vtksys::SystemTools::DuplicateString(fname);
  this->Modified();
  this->UpdateNeeded = true;
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::SetSourceProcess(int proc)
{
  if (this->SourceProcess != proc)
    {
    this->SourceProcess = proc;
    this->Modified();
    this->UpdateNeeded = true;
    }
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  if (this->UpdateNeeded && !this->ForceNoUpdates)
    {
    this->UpdateImage();
    }
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::ReviveVTKObjects()
{
  this->Superclass::ReviveVTKObjects();
  // When loading revival state, assume that the image is loaded correctly.
  this->ForceNoUpdates = true;
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::UpdateImage()
{
  if (!this->FileName)
    {
    return;
    }

  if ((this->SourceProcess & this->Servers) == 0)
    {
    vtkErrorMacro("The proxy VTK objects have not been created on the processes "
      "where the image file is present.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID() << "ReadImageFromFile"
          << this->FileName 
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::GetRootId(this->SourceProcess), stream);

  int readable = 0;
  if(!pm->GetLastResult(this->ConnectionID,
      vtkProcessModule::GetRootId(this->SourceProcess)).GetArgument(0, 0, &readable) ||
    !readable)
    {
    vtkErrorMacro("Cannot read file " << this->FileName << "on the process indicated.");
    return;
    }

  stream  << vtkClientServerStream::Invoke
          << this->GetID() << "GetImageAsString"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::GetRootId(this->SourceProcess), stream);

  vtkClientServerStream reply;
  int retVal = pm->GetLastResult(this->ConnectionID,
    vtkProcessModule::GetRootId(this->SourceProcess)).GetArgument(0, 0, &reply);

  if(!retVal)
    {
    vtkErrorMacro("Error getting reply from server.");
    return;
    }

  // Now transmit the reply to all the processes where the image is not
  // present.
  stream  << vtkClientServerStream::Invoke
          << this->GetID() << "ReadImageFromString"
          << reply
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  reply.Reset();

  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "ClearBuffers"
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  // just to ensure that last result cache is released.
  pm->GetLastResult(this->ConnectionID, this->SourceProcess);

  this->UpdateNeeded = false;
}

//----------------------------------------------------------------------------
void vtkSMNetworkImageSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
    << (this->FileName? this->FileName : "(none)") << endl;
}


