/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayWindowProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayWindowProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMDisplayerProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMDisplayWindowProxy);
vtkCxxRevisionMacro(vtkSMDisplayWindowProxy, "1.6");

//---------------------------------------------------------------------------
vtkSMDisplayWindowProxy::vtkSMDisplayWindowProxy()
{
  this->SetVTKClassName("vtkRenderWindow");

  // TODO revise this: where should windowtoimage be?
  this->WindowToImage = vtkSMProxy::New();
  this->WindowToImage->ClearServerIDs();
  this->WindowToImage->AddServerID(0);

}

//---------------------------------------------------------------------------
vtkSMDisplayWindowProxy::~vtkSMDisplayWindowProxy()
{
  this->WindowToImage->Delete();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();

  vtkClientServerStream str;
  int i;

  // TODO revise
  // These are good defaults for batch scripting but should
  // be made more general for other uses.
  for (i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->GetID(i) << "DoubleBufferOff"
        << vtkClientServerStream::End;
    }

  vtkSMProxy* rendererProxy = this->GetSubProxy("renderer");
  if (!rendererProxy)
    {
    vtkErrorMacro("No renderer sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << this->GetID(i) << "AddRenderer" << rendererProxy->GetID(i)
          << vtkClientServerStream::End;
      }
    }

  vtkSMProxy* cameraProxy = this->GetSubProxy("camera");
  if (!cameraProxy)
    {
    vtkErrorMacro("No camera sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << rendererProxy->GetID(i) 
          << "SetActiveCamera" 
          << cameraProxy->GetID(i)
          << vtkClientServerStream::End;
      }
    }

  vtkSMProxy* compositeProxy = this->GetSubProxy("composite");
  if (!compositeProxy)
    {
    vtkErrorMacro("No composite sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    }
  else
    {
    for (i=0; i<numObjects; i++)
      {
      str << vtkClientServerStream::Invoke 
          << compositeProxy->GetID(i) 
          << "SetRenderWindow" 
          << this->GetID(i)
          << vtkClientServerStream::End;
      str << vtkClientServerStream::Invoke 
          << compositeProxy->GetID(i) 
          << "InitializeRMIs" 
          << vtkClientServerStream::End;
      }
    }

  if (str.GetNumberOfMessages() > 0)
    {
    cm->SendStreamToServers(&str, 
                            this->GetNumberOfServerIDs(),
                            this->GetServerIDs());
    }

  str.Reset();

  this->WindowToImage->SetVTKClassName("vtkWindowToImageFilter");
  this->WindowToImage->CreateVTKObjects(1);
  
  if (numObjects > 0)
    {
    str << vtkClientServerStream::Invoke 
        << this->WindowToImage->GetID(0) 
        << "SetInput" 
        << this->GetID(0)
        << vtkClientServerStream::End;
    }
  

  cm->SendStreamToServers(&str, 
                          this->WindowToImage->GetNumberOfServerIDs(),
                          this->WindowToImage->GetServerIDs());
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::WriteImage(const char* filename,
                                         const char* writerName)
{
  if (!filename || !writerName)
    {
    return;
    }

  vtkClientServerStream str;

  vtkSMProxy* imageWriter = vtkSMProxy::New();
  imageWriter->ClearServerIDs();
  imageWriter->AddServerID(0);

  imageWriter->SetVTKClassName(writerName);
  imageWriter->CreateVTKObjects(1);
  
  str << vtkClientServerStream::Invoke 
      << this->WindowToImage->GetID(0) 
      << "GetOutput" 
      << vtkClientServerStream::End;
  
  str << vtkClientServerStream::Invoke 
      << imageWriter->GetID(0) 
      << "SetInput" 
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
      << this->WindowToImage->GetID(0) 
      << "Modified" 
      << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
      << imageWriter->GetID(0) 
      << "SetFileName" 
      << filename
      << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke 
      << imageWriter->GetID(0) 
      << "Write" 
      << vtkClientServerStream::End;

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&str, 
                          imageWriter->GetNumberOfServerIDs(),
                          imageWriter->GetServerIDs());
  str.Reset();

  imageWriter->Delete();

}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::AddDisplayer(vtkSMDisplayerProxy* display)
{
  this->CreateVTKObjects(1);

  vtkSMProxy* actorProxy = display->GetSubProxy("actor");
  if (!actorProxy)
    {
    vtkErrorMacro("No actor sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkSMProxy* rendererProxy = this->GetSubProxy("renderer");
  if (!rendererProxy)
    {
    vtkErrorMacro("No renderer sub-proxy was defined. Please make sure that "
                  "the configuration file defines it.");
    return;
    }

  vtkClientServerStream str;
  int numActors = actorProxy->GetNumberOfIDs();
  for (int i=0; i<numActors; i++)
    {
    str << vtkClientServerStream::Invoke 
        << rendererProxy->GetID(0) 
        << "AddActor" 
        << actorProxy->GetID(i)
        << vtkClientServerStream::End;
    }
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&str, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  str.Reset();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::StillRender()
{
  int numObjects = this->GetNumberOfIDs();
  vtkClientServerStream str;
  for (int i=0; i<numObjects; i++)
    {
    str << vtkClientServerStream::Invoke 
        << this->GetID(i) << "Render"
        << vtkClientServerStream::End;
    }
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  // Call render on the client only. The composite manager should
  // take care of the rest.
  int serverids = 0;
  cm->SendStreamToServers(&str, 
                          1,
                          &serverids);
  str.Reset();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::InteractiveRender()
{
  // TODO implement this
  this->StillRender();
}

//---------------------------------------------------------------------------
void vtkSMDisplayWindowProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
