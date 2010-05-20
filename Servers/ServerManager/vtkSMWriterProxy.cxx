/*=========================================================================

Program:   ParaView
Module:    vtkSMWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMWriterProxy);
//-----------------------------------------------------------------------------
vtkSMWriterProxy::vtkSMWriterProxy()
{
  this->ErrorCode = vtkErrorCode::NoError;
  this->SupportsParallel = 0;
  this->ParallelOnly = 0;
  this->FileNameMethod = 0;
}

//-----------------------------------------------------------------------------
vtkSMWriterProxy::~vtkSMWriterProxy()
{
  this->SetFileNameMethod(0);
}

//-----------------------------------------------------------------------------
int vtkSMWriterProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
  vtkPVXMLElement* element)
{
  if (element->GetAttribute("supports_parallel"))
    {
    element->GetScalarAttribute("supports_parallel", &this->SupportsParallel);
    }

  if (element->GetAttribute("parallel_only"))
    {
    element->GetScalarAttribute("parallel_only", &this->ParallelOnly);
    }

  if (this->ParallelOnly)
    {
    this->SetSupportsParallel(1);
      // if ParallelOnly, then we must support Parallel.
    }

  const char* setFileNameMethod = element->GetAttribute("file_name_method");
  if (setFileNameMethod)
    {
    this->SetFileNameMethod(setFileNameMethod);
    }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkSMProxy* writer = this->GetSubProxy("Writer");
  if (!writer)
    {
    // if no writer is present, then this is not a meta writer.
    return;
    }
  
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
    << this->GetID() << "SetWriter" << writer->GetID() 
    << vtkClientServerStream::End;
  if (this->GetFileNameMethod())
    {
    stream << vtkClientServerStream::Invoke
      << this->GetID() 
      << "SetFileNameMethod" 
      << this->GetFileNameMethod()
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline()
{
  this->Superclass::UpdatePipeline();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "GetErrorCode"
      << vtkClientServerStream::End;

  pm->SendPrepareProgress(this->ConnectionID);
  pm->SendStream(this->ConnectionID, this->Servers, str);
  pm->GetLastResult(this->GetConnectionID(), this->GetServers()).GetArgument(
    0, 0, &this->ErrorCode);
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline(double time)
{
  this->Superclass::UpdatePipeline(time);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "GetErrorCode"
      << vtkClientServerStream::End;
  
  pm->SendPrepareProgress(this->ConnectionID);
  pm->SendStream(this->ConnectionID, this->Servers, str);
  pm->GetLastResult(this->GetConnectionID(), this->GetServers()).GetArgument(
    0, 0, &this->ErrorCode);
  pm->SendCleanupPendingProgress(this->ConnectionID);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::AddInput(unsigned int inputPort,
                                 vtkSMSourceProxy* input, 
                                 unsigned int outputPort,
                                 const char* method)
{

  vtkSMSourceProxy* completeArrays = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("CompleteArrays"));
  if (completeArrays)
    {

    vtkSMInputProperty* ivp  = vtkSMInputProperty::SafeDownCast(
      completeArrays->GetProperty("Input"));
    ivp->RemoveAllProxies();
    ivp->AddInputConnection(input, outputPort);
    input = completeArrays; // change the actual input to the writer to be
      // output of complete arrays.
    outputPort = 0; // since input changed, outputPort of the  input 
                    // should also change.
    completeArrays->UpdateVTKObjects();

    }

  this->Superclass::AddInput(inputPort, input, outputPort, method);
}


//-----------------------------------------------------------------------------
void vtkSMWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ErrorCode: " 
     << vtkErrorCode::GetStringFromErrorCode(this->ErrorCode) << endl;
  os << indent << "SupportsParallel: "
     << this->SupportsParallel << endl;
  os << indent << "ParallelOnly: "
     << this->ParallelOnly << endl;
}
