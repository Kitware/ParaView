/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessModule.h"

#include "vtkCallbackCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformation.h"
#include "vtkToolkits.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

vtkProcessModule* vtkProcessModule::ProcessModule = 0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProcessModule);
vtkCxxRevisionMacro(vtkProcessModule, "1.3");

//----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  this->UniqueID.ID = 3;
  this->Controller = NULL;
  this->TemporaryInformation = NULL;
  this->ClientServerStream = 0;
  this->Interpreter = 0;
  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;
}

//----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  // Free Interpreter and ClientServerStream.
  this->FinalizeInterpreter();

  // Other cleanup.
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::ProcessModule;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetProcessModule(vtkProcessModule* pm)
{
  vtkProcessModule::ProcessModule = pm;
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformationRenderServer(vtkPVInformation* ,
                                                       vtkClientServerID )
{
  vtkErrorMacro("This should only be called from the client of a client render server mode paraview");
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformation(vtkPVInformation* info,
                                           vtkClientServerID id)
{
  // Just a simple way of passing the information object to the next
  // method.
  this->TemporaryInformation = info;
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "GatherInformationInternal" << info->GetClassName() << id
    << vtkClientServerStream::End;
  this->SendStreamToClientAndServer();
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
void vtkProcessModule::GatherInformationInternal(const char*,
                                                   vtkObject* object)
{
  // This class is used only for one processes.
  if (this->TemporaryInformation == NULL)
    {
    vtkErrorMacro("Information argument not set.");
    return;
    }
  if (object == NULL)
    {
    vtkErrorMacro("Object tcl name must be wrong.");
    return;
    }

  this->TemporaryInformation->CopyFromObject(object);
 }

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToClient()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToClientAndServer()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToClientAndServerRoot()
{
  this->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToRenderServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}


//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToRenderServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}


//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToRenderServerAndServerRoot()
{ 
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToRenderServerAndServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToClientAndRenderServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToRenderServerClientAndServer()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendStreamToClientAndRenderServerRoot()
{
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset(); 
}

//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::NewStreamObject(const char* type)
{
  vtkClientServerStream& stream = this->GetStream();
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkProcessModule::GetObjectFromID(vtkClientServerID id)
{
  return this->Interpreter->GetObjectFromID(id);
}

//----------------------------------------------------------------------------
void vtkProcessModule::DeleteStreamObject(vtkClientServerID id)
{
  vtkClientServerStream& stream = this->GetStream();
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastServerResult()
{
  if(this->Interpreter)
    {
    return this->Interpreter->GetLastResult();
    }
  else
    {
    static vtkClientServerStream emptyResult;
    return emptyResult;
    }
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastClientResult()
{
  return this->GetLastServerResult();
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter* vtkProcessModule::GetInterpreter()
{
  return this->Interpreter;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetUniqueID()
{
  this->UniqueID.ID++;
  return this->UniqueID;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetProcessModuleID()
{
  vtkClientServerID id = {2};
  return id;
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "ReportInterpreterErrors: "
     << this->ReportInterpreterErrors << endl;
}

//----------------------------------------------------------------------------
void vtkProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();
  this->ClientServerStream = new vtkClientServerStream;
  
  // Setup a callback for the interpreter to report errors.
  this->InterpreterObserver = vtkCallbackCommand::New();
  this->InterpreterObserver->SetCallback(&vtkProcessModule::InterpreterCallbackFunction);
  this->InterpreterObserver->SetClientData(this);
  this->Interpreter->AddObserver(vtkCommand::UserEvent,
                                 this->InterpreterObserver);

  // Assign standard IDs.
// TODO move this to subclass
//   vtkPVApplication *app = this->GetPVApplication();
  vtkClientServerStream css;
//   css << vtkClientServerStream::Assign
//       << this->GetApplicationID() << app
//       << vtkClientServerStream::End;
  css << vtkClientServerStream::Assign
      << this->GetProcessModuleID() << this
      << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);
}

//----------------------------------------------------------------------------
void vtkProcessModule::FinalizeInterpreter()
{
  if(!this->Interpreter)
    {
    return;
    }

  // Delete the standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Delete
      << this->GetProcessModuleID()
      << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  // Free the interpreter and supporting stream.
  this->Interpreter->RemoveObserver(this->InterpreterObserver);
  this->InterpreterObserver->Delete();
  delete this->ClientServerStream;
  this->Interpreter->Delete();
  this->Interpreter = 0;
}

//----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallbackFunction(vtkObject*,
                                                     unsigned long eid,
                                                     void* cd, void* d)
{
  reinterpret_cast<vtkProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallback(unsigned long, void* pinfo)
{
  if(!this->ReportInterpreterErrors)
    {
    return;
    }

  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(pinfo);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
     (last.GetCommand(0) == vtkClientServerStream::Error) &&
     last.GetArgument(0, 0, &errorMessage))
    {
    ostrstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str());
    cerr << errorMessage << endl;
    error.rdbuf()->freeze(0);
    vtkErrorMacro("Aborting execution for debugging purposes.");
    abort();
    }
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStringToClient(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClient();
  return 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStringToClientAndServer(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClientAndServer();
  return 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStringToClientAndServerRoot(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToClientAndServerRoot();
  return 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStringToServer(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToServer();
  return 1;
}

//----------------------------------------------------------------------------
int vtkProcessModule::SendStringToServerRoot(const char* str)
{
  if(!this->ClientServerStream->StreamFromString(str))
    {
    return 0;
    }
  this->SendStreamToServerRoot();
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkProcessModule::GetStringFromServer()
{
  return this->GetLastServerResult().StreamToString();
}

//----------------------------------------------------------------------------
const char* vtkProcessModule::GetStringFromClient()
{
  return this->GetLastClientResult().StreamToString();
}
